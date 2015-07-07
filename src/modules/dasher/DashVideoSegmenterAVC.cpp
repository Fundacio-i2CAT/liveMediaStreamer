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
 *
 */

 #include "DashVideoSegmenterAVC.hh"

DashVideoSegmenterAVC::DashVideoSegmenterAVC(std::chrono::seconds segDur) : 
DashVideoSegmenter(segDur, VIDEO_CODEC_AVC), 
updatedSPS(false), updatedPPS(false),
isVCL(false)
{

}

DashVideoSegmenterAVC::~DashVideoSegmenterAVC()
{

}

bool DashVideoSegmenterAVC::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, VIDEO_TYPE_AVC);
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

bool DashVideoSegmenterAVC::updateMetadata()
{
    if (!updatedSPS || !updatedPPS) {
        return false;
    }

    createMetadata();
    updatedSPS = false;
    updatedPPS = false;
    return true;
}

bool DashVideoSegmenterAVC::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, VIDEO_TYPE_AVC);
    return true;
}

bool DashVideoSegmenterAVC::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, bool &newFrame)
{
    unsigned char nalType;

    nalType = nalData[0] & H264_NALU_TYPE_MASK;

    switch (nalType) {
        case SPS_AVC:
            saveSPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case PPS_AVC:
            savePPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case SEI:
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case AUD_AVC:
            newFrame = isVCL;
            break;
        case IDR:
            isVCL = true;
            isIntra = true;
            newFrame = false;
            break;
        case NON_IDR:
            isVCL = true;
            isIntra = false;
            newFrame = false;
            break;
        default:
            utils::errorMsg("Error parsing NAL: NalType " + std::to_string(nalType) + " not contemplated");
            return false;
    }

    if (nalType == IDR || nalType == NON_IDR) {
        frameData.insert(frameData.end(), (nalDataLength >> 24) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 16) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 8) & 0xFF);
        frameData.insert(frameData.end(), nalDataLength & 0xFF);

        frameData.insert(frameData.end(), nalData, nalData + nalDataLength);
    }

    return true;
}


void DashVideoSegmenterAVC::saveSPS(unsigned char* data, int dataLength)
{
    lastSPS.clear();
    lastSPS.insert(lastSPS.begin(), data, data + dataLength);
    updatedSPS = true;
}

void DashVideoSegmenterAVC::savePPS(unsigned char* data, int dataLength)
{
    lastPPS.clear();
    lastPPS.insert(lastPPS.begin(), data, data + dataLength);
    updatedPPS = true;
}

void DashVideoSegmenterAVC::createMetadata()
{
    int spsLength = lastSPS.size();
    int ppsLength = lastPPS.size();

    metadata.clear();

    metadata.insert(metadata.end(), H264_METADATA_VERSION_FLAG);
    metadata.insert(metadata.end(), lastSPS.begin(), lastSPS.begin() + 3);
    metadata.insert(metadata.end(), METADATA_RESERVED_BYTES1 + AVCC_HEADER_BYTES_MINUS_ONE);
    metadata.insert(metadata.end(), METADATA_RESERVED_BYTES2 + NUMBER_OF_SPS);
    metadata.insert(metadata.end(), (spsLength >> 8) & 0xFF);
    metadata.insert(metadata.end(), spsLength & 0xFF);
    metadata.insert(metadata.end(), lastSPS.begin(), lastSPS.end());
    metadata.insert(metadata.end(), NUMBER_OF_PPS);
    metadata.insert(metadata.end(), (ppsLength >> 8) & 0xFF);
    metadata.insert(metadata.end(), ppsLength & 0xFF);
    metadata.insert(metadata.end(), lastPPS.begin(), lastPPS.end());
}
