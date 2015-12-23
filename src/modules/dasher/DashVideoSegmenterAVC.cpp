/*
 *  DashVideoSegmenterAVC.cpp - DASH AVC video stream segmenter
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

#include "DashVideoSegmenterAVC.hh"

DashVideoSegmenterAVC::DashVideoSegmenterAVC(std::chrono::seconds segDur) : 
DashVideoSegmenter(segDur, VIDEO_CODEC_AVC)
{
    vFrame = InterleavedVideoFrame::createNew(H264, LENGTH_H264_FRAME);
    tmpFrame = InterleavedVideoFrame::createNew(H264, LENGTH_H264_FRAME);
}

DashVideoSegmenterAVC::~DashVideoSegmenterAVC()
{

}

uint8_t DashVideoSegmenterAVC::generateContext()
{
    return generate_context(&dashContext, VIDEO_TYPE_AVC);
}

void DashVideoSegmenterAVC::updateExtradata()
{
    if (sps.empty() || pps.empty()) {
        return;
    }

    createExtradata();
    sps.clear();
    pps.clear();
    return;
}

bool DashVideoSegmenterAVC::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, VIDEO_TYPE_AVC);
    return true;
}

VideoFrame* DashVideoSegmenterAVC::parseNal(VideoFrame* nal)
{
    int startCodeOffset;
    unsigned char* nalData;
    unsigned nalDataLength;
    unsigned char nalType;
    bool newFrame = false;

    startCodeOffset = detectStartCode(nal->getDataBuf());
    nalData = nal->getDataBuf() + startCodeOffset;
    nalDataLength = nal->getLength() - startCodeOffset;

    if (!nalData || nalDataLength <= 0) {
        utils::errorMsg("Error parsing NAL: invalid data pointer or length");
        return NULL;
    }

    nalType = nalData[0] & H264_NALU_TYPE_MASK;
    
    previousIntra = currentIntra;

    switch (nalType) {
        case SPS_AVC:
            saveSPS(nalData, nalDataLength);
            currentIntra = false;
            break;
        case PPS_AVC:
            savePPS(nalData, nalDataLength);
            currentIntra = false;
            break;
        case SEI:
            currentIntra = false;
            break;
        case IDR:
            currentIntra = true;
            break;
        case NON_IDR:
            currentIntra = false;
            break;
        case AUD_AVC:
            break;
        default:
            utils::errorMsg("Error parsing NAL: NalType " + std::to_string(nalType) + " not contemplated");
            return NULL;
    }
    
    if ((nalType == AUD_AVC || nal->getPresentationTime() > tmpFrame->getPresentationTime()) && tmpFrame->getLength() > 0){
        std::swap(tmpFrame, vFrame);
        resetFrame();
        newFrame = true;
    }

    if ((nalType == IDR || nalType == NON_IDR) &&
        !appendNalToFrame(tmpFrame, nalData, nalDataLength, nal->getWidth(), nal->getHeight(), nal->getPresentationTime())) { 
        utils::errorMsg("[DashVideoSegmenterHEVC::parseNal] Error appending NAL to frame");
    }

    if (newFrame){
        return vFrame;
    }
    
    return NULL;
}

void DashVideoSegmenterAVC::saveSPS(unsigned char* data, int dataLength)
{
    sps.clear();
    sps.insert(sps.begin(), data, data + dataLength);
}

void DashVideoSegmenterAVC::savePPS(unsigned char* data, int dataLength)
{
    pps.clear();
    pps.insert(pps.begin(), data, data + dataLength);
}

void DashVideoSegmenterAVC::createExtradata()
{
    int spsLength = sps.size();
    int ppsLength = pps.size();

    extradata.clear();

    extradata.insert(extradata.end(), H264_METADATA_VERSION_FLAG);
    extradata.insert(extradata.end(), sps.begin(), sps.begin() + 3);
    extradata.insert(extradata.end(), METADATA_RESERVED_BYTES1 + AVCC_HEADER_BYTES_MINUS_ONE);
    extradata.insert(extradata.end(), METADATA_RESERVED_BYTES2 + NUMBER_OF_SPS);
    extradata.insert(extradata.end(), (spsLength >> 8) & 0xFF);
    extradata.insert(extradata.end(), spsLength & 0xFF);
    extradata.insert(extradata.end(), sps.begin(), sps.end());
    extradata.insert(extradata.end(), NUMBER_OF_PPS);
    extradata.insert(extradata.end(), (ppsLength >> 8) & 0xFF);
    extradata.insert(extradata.end(), ppsLength & 0xFF);
    extradata.insert(extradata.end(), pps.begin(), pps.end());
}
