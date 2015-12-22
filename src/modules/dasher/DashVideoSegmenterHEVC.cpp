/*
 *  DashVideoSegmenterHEVC.cpp - DASH HEVC video stream segmenter
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Gerard Castillo <gerard.castillo@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net> 
 */

#include "DashVideoSegmenterHEVC.hh"

DashVideoSegmenterHEVC::DashVideoSegmenterHEVC(std::chrono::seconds segDur, std::chrono::microseconds& offset) : 
DashVideoSegmenter(segDur, VIDEO_CODEC_HEVC, offset)
{
    vFrame = InterleavedVideoFrame::createNew(H265, LENGTH_H264_FRAME);
    tmpFrame = InterleavedVideoFrame::createNew(H265, LENGTH_H264_FRAME);

}

DashVideoSegmenterHEVC::~DashVideoSegmenterHEVC()
{

}

uint8_t DashVideoSegmenterHEVC::generateContext()
{
    return generate_context(&dashContext, VIDEO_TYPE_HEVC);
}

void DashVideoSegmenterHEVC::updateExtradata()
{
    if (vps.empty() || sps.empty() || pps.empty()) {
        return;
    }

    createExtradata();
    vps.clear();
    sps.clear();
    pps.clear();
    return;
}

bool DashVideoSegmenterHEVC::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, VIDEO_TYPE_HEVC);
    return true;
}

VideoFrame* DashVideoSegmenterHEVC::parseNal(VideoFrame* nal)
{
    int startCodeOffset;
    unsigned char* nalData;
    int nalDataLength;
    unsigned char nalType;
    bool newFrame = false;

    startCodeOffset = detectStartCode(nal->getDataBuf());
    nalData = nal->getDataBuf() + startCodeOffset;
    nalDataLength = nal->getLength() - startCodeOffset;

    if (!nalData || nalDataLength <= 0) {
        utils::errorMsg("Error parsing NAL: invalid data pointer or length");
        return NULL;
    }

    nalType = (nalData[0] & H265_NALU_TYPE_MASK) >> 1;
    
    previousIntra = currentIntra;

    switch (nalType) {
        case VPS:
            saveVPS(nalData, nalDataLength);
            currentIntra = false;
            break;
        case SPS_HEVC:
            saveSPS(nalData, nalDataLength);
            currentIntra = false;
            break;
        case PPS_HEVC:
            savePPS(nalData, nalDataLength);
            currentIntra = false;
            break;
        case PREFIX_SEI_NUT:
        case SUFFIX_SEI_NUT:
            currentIntra = false;
            break;
        case AUD_HEVC:
            break;
        case IDR1:
        case IDR2:
        case CRA:
            currentIntra = true;
            break;
        case NON_TSA_STSA_0:
        case NON_TSA_STSA_1:
            currentIntra = false;
            break;
        default:
            utils::errorMsg("Error parsing NAL: NalType " + std::to_string(nalType) + " not contemplated");
            return NULL;
    }
    
    if (nalType == AUD_HEVC || nal->getPresentationTime() > tmpFrame->getPresentationTime()){
        std::swap(tmpFrame,vFrame);
        resetFrame();
        newFrame = true;
    }

    if ((nalType == IDR1 || nalType == IDR2 || nalType == CRA 
        || nalType == NON_TSA_STSA_0 || nalType == NON_TSA_STSA_1) && 
        !appendNalToFrame(tmpFrame, nalData, nalDataLength, nal->getWidth(), nal->getHeight(), nal->getPresentationTime())) {
        utils::errorMsg("[DashVideoSegmenterHEVC::parseNal] Error appending NAL to frame");
    }

    if (newFrame){
        return vFrame;
    }
    
    return NULL;
}

void DashVideoSegmenterHEVC::saveVPS(unsigned char* data, int dataLength)
{
    vps.clear();
    vps.insert(vps.begin(), data, data + dataLength);
}

void DashVideoSegmenterHEVC::saveSPS(unsigned char* data, int dataLength)
{
    sps.clear();
    sps.insert(sps.begin(), data, data + dataLength);
}

void DashVideoSegmenterHEVC::savePPS(unsigned char* data, int dataLength)
{
    pps.clear();
    pps.insert(pps.begin(), data, data + dataLength);
}

void DashVideoSegmenterHEVC::createExtradata()
{
    int vpsLength = vps.size();
    int spsLength = sps.size();
    int ppsLength = pps.size();

    extradata.clear();

    extradata.insert(extradata.end(), H265_METADATA_VERSION_FLAG);
    extradata.insert(extradata.end(), H265_METADATA_CONFIG_FLAGS);
    extradata.insert(extradata.end(), H265_METADATA_PROFILE_COMPATIBILITY_FLAGS);
    extradata.insert(extradata.end(), H265_METADATA_PROFILE_COMPATIBILITY_FLAGS_PADDING, 0x00);
    extradata.insert(extradata.end(), H265_METADATA_CONSTRAINT_INDICATOR_FLAGS);
    extradata.insert(extradata.end(), H265_METADATA_CONSTRAINT_INDICATOR_FLAGS_PADDING, 0x00);
    extradata.insert(extradata.end(), H265_METADATA_GENERAL_LEVEL_IDC);

    extradata.insert(extradata.end(), (H265_METADATA_MIN_SPATIAL_SEGMENTATION >> 8) | H265_METADATA_MIN_SPATIAL_SEGMENTATION_RESERVED_BYTES);
    extradata.insert(extradata.end(), H265_METADATA_MIN_SPATIAL_SEGMENTATION);
    extradata.insert(extradata.end(), H265_METADATA_PARALLELISM_TYPE_RESERVED_BYTES | H265_METADATA_PARALLELISM_TYPE);
    extradata.insert(extradata.end(), H265_METADATA_CHROMA_FORMAT_RESERVED_BYTES | H265_METADATA_CHROMA_FORMAT);
    extradata.insert(extradata.end(), H265_METADATA_BIT_DEPTH_LUMA_MINUS_8_RESERVED_BYTES | H265_METADATA_BIT_DEPTH_LUMA_MINUS_8);
    extradata.insert(extradata.end(), H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8_RESERVED_BYTES | H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8);
    extradata.insert(extradata.end(), H265_METADATA_AVG_FRAMERATE >> 8);
    extradata.insert(extradata.end(), H265_METADATA_AVG_FRAMERATE);
    extradata.insert(extradata.end(), 
        (H265_METADATA_CTX_FRAMERATE << 6) | 
        (H265_METADATA_NUM_TEMPORAL_LAYERS << 3) |
        (H265_METADATA_TEMPORAL_ID_NESTED << 2) |
        (H265_METADATA_LENGTH_SIZE_MINUS_ONE)
    );
    extradata.insert(extradata.end(), H265_NUMBER_OF_ARRAYS);

    extradata.insert(extradata.end(), VPS);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY);
    extradata.insert(extradata.end(), (vpsLength >> 8) & 0xFF);
    extradata.insert(extradata.end(), vpsLength & 0xFF);
    extradata.insert(extradata.end(), vps.begin(), vps.end());

    extradata.insert(extradata.end(), SPS_HEVC);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY);
    extradata.insert(extradata.end(), (spsLength >> 8) & 0xFF);
    extradata.insert(extradata.end(), spsLength & 0xFF);
    extradata.insert(extradata.end(), sps.begin(), sps.end());

    extradata.insert(extradata.end(), PPS_HEVC);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    extradata.insert(extradata.end(), H265_NUM_NALUS_IN_ARRAY);
    extradata.insert(extradata.end(), (ppsLength >> 8) & 0xFF);
    extradata.insert(extradata.end(), ppsLength & 0xFF);
    extradata.insert(extradata.end(), pps.begin(), pps.end());
}

