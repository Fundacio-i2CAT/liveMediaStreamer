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
 *
 */

 #include "DashVideoSegmenterHEVC.hh"

DashVideoSegmenterHEVC::DashVideoSegmenterHEVC(std::chrono::seconds segDur) : 
DashVideoSegmenter(segDur, VIDEO_CODEC_HEVC), 
updatedSPS(false), updatedPPS(false),
isVCL(false)
{

}

DashVideoSegmenterHEVC::~DashVideoSegmenterHEVC()
{

}

bool DashVideoSegmenterHEVC::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, VIDEO_TYPE_HEVC);
    }

    if (i2error != I2OK || !dashContext) {
        return false;
    }

    i2error = fill_video_context(&dashContext, width, height, framerate, timeBase, sampleDuration);

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segmentDuration, &dashContext);
    return true;
}

bool DashVideoSegmenterHEVC::updateMetadata()
{
    if (!updatedVPS || !updatedSPS || !updatedPPS) {
        return false;
    }

    createMetadata();
    updatedVPS = false;
    updatedSPS = false;
    updatedPPS = false;
    return true;
}

bool DashVideoSegmenterHEVC::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, VIDEO_TYPE_HEVC);
    return true;
}

bool DashVideoSegmenterHEVC::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, bool &newFrame)
{
    unsigned char nalType;

    nalType = (nalData[0] & H265_NALU_TYPE_MASK) >> 1;

    switch (nalType) {
        case VPS:
            saveVPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case SPS_HEVC:
            saveSPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case PPS_HEVC:
            savePPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case PREFIX_SEI_NUT:
        case SUFFIX_SEI_NUT:
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case AUD_HEVC:
            newFrame = isVCL;
            break;
        case IDR1:
        case IDR2:
        case CRA:
            isVCL = true;
            isIntra = true;
            newFrame = false;
            break;
        case NON_TSA_STSA_0:
        case NON_TSA_STSA_1:
            isVCL = true;
            isIntra = false;
            newFrame = false;
            break;
        default:
            utils::errorMsg("Error parsing NAL: NalType " + std::to_string(nalType) + " not contemplated");
            return false;
    }

    //TODO: set as function
    if (nalType == IDR1 || nalType == IDR2 || nalType == CRA || nalType == NON_TSA_STSA_0 || nalType == NON_TSA_STSA_1) {
        frameData.insert(frameData.end(), (nalDataLength >> 24) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 16) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 8) & 0xFF);
        frameData.insert(frameData.end(), nalDataLength & 0xFF);

        frameData.insert(frameData.end(), nalData, nalData + nalDataLength);
    }

    return true;
}

void DashVideoSegmenterHEVC::saveVPS(unsigned char* data, int dataLength)
{
    lastVPS.clear();
    lastVPS.insert(lastVPS.begin(), data, data + dataLength);
    updatedVPS = true;
}

void DashVideoSegmenterHEVC::saveSPS(unsigned char* data, int dataLength)
{
    lastSPS.clear();
    lastSPS.insert(lastSPS.begin(), data, data + dataLength);
    updatedSPS = true;
}

void DashVideoSegmenterHEVC::savePPS(unsigned char* data, int dataLength)
{
    lastPPS.clear();
    lastPPS.insert(lastPPS.begin(), data, data + dataLength);
    updatedPPS = true;
}

void DashVideoSegmenterHEVC::createMetadata()
{
    int vpsLength = lastVPS.size();
    int spsLength = lastSPS.size();
    int ppsLength = lastPPS.size();

    metadata.clear();

    metadata.insert(metadata.end(), H265_METADATA_VERSION_FLAG);
    metadata.insert(metadata.end(), H265_METADATA_CONFIG_FLAGS);
    metadata.insert(metadata.end(), H265_METADATA_PROFILE_COMPATIBILITY_FLAGS);
    metadata.insert(metadata.end(), H265_METADATA_PROFILE_COMPATIBILITY_FLAGS_PADDING, 0x00);
    metadata.insert(metadata.end(), H265_METADATA_CONSTRAINT_INDICATOR_FLAGS);
    metadata.insert(metadata.end(), H265_METADATA_CONSTRAINT_INDICATOR_FLAGS_PADDING, 0x00);
    metadata.insert(metadata.end(), H265_METADATA_GENERAL_LEVEL_IDC);

    metadata.insert(metadata.end(), (H265_METADATA_MIN_SPATIAL_SEGMENTATION >> 8) | H265_METADATA_MIN_SPATIAL_SEGMENTATION_RESERVED_BYTES);
    metadata.insert(metadata.end(), H265_METADATA_MIN_SPATIAL_SEGMENTATION);
    metadata.insert(metadata.end(), H265_METADATA_PARALLELISM_TYPE_RESERVED_BYTES | H265_METADATA_PARALLELISM_TYPE);
    metadata.insert(metadata.end(), H265_METADATA_CHROMA_FORMAT_RESERVED_BYTES | H265_METADATA_CHROMA_FORMAT);
    metadata.insert(metadata.end(), H265_METADATA_BIT_DEPTH_LUMA_MINUS_8_RESERVED_BYTES | H265_METADATA_BIT_DEPTH_LUMA_MINUS_8);
    metadata.insert(metadata.end(), H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8_RESERVED_BYTES | H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8);
    metadata.insert(metadata.end(), H265_METADATA_AVG_FRAMERATE >> 8);
    metadata.insert(metadata.end(), H265_METADATA_AVG_FRAMERATE);
    metadata.insert(metadata.end(), 
        (H265_METADATA_CTX_FRAMERATE << 6) | 
        (H265_METADATA_NUM_TEMPORAL_LAYERS << 3) |
        (H265_METADATA_TEMPORAL_ID_NESTED << 2) |
        (H265_METADATA_LENGTH_SIZE_MINUS_ONE)
    );
    metadata.insert(metadata.end(), H265_NUMBER_OF_ARRAYS);

    metadata.insert(metadata.end(), VPS);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY);
    metadata.insert(metadata.end(), (vpsLength >> 8) & 0xFF);
    metadata.insert(metadata.end(), vpsLength & 0xFF);
    metadata.insert(metadata.end(), lastVPS.begin(), lastVPS.end());

    metadata.insert(metadata.end(), SPS_HEVC);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY);
    metadata.insert(metadata.end(), (spsLength >> 8) & 0xFF);
    metadata.insert(metadata.end(), spsLength & 0xFF);
    metadata.insert(metadata.end(), lastSPS.begin(), lastSPS.end());

    metadata.insert(metadata.end(), PPS_HEVC);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY >> 8);
    metadata.insert(metadata.end(), H265_NUM_NALUS_IN_ARRAY);
    metadata.insert(metadata.end(), (ppsLength >> 8) & 0xFF);
    metadata.insert(metadata.end(), ppsLength & 0xFF);
    metadata.insert(metadata.end(), lastPPS.begin(), lastPPS.end());
}
