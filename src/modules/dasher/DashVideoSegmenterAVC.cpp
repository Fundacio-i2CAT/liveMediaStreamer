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
    vFrame = InterleavedVideoFrame::createNew(H264, 5000000);
}

DashVideoSegmenterAVC::~DashVideoSegmenterAVC()
{

}

uint8_t DashVideoSegmenterAVC::generateContext()
{
    return generate_context(&dashContext, VIDEO_TYPE_AVC);
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

bool DashVideoSegmenterAVC::parseNal(VideoFrame* nal, bool &newFrame)
{
    int startCodeOffset;
    unsigned char* nalData;
    unsigned nalDataLength;
    unsigned char nalType;

    startCodeOffset = detectStartCode(nal->getDataBuf());
    nalData = nal->getDataBuf() + startCodeOffset;
    nalDataLength = nal->getLength() - startCodeOffset;

    if (!nalData || nalDataLength <= 0) {
        utils::errorMsg("Error parsing NAL: invalid data pointer or length");
        return false;
    }

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

    //TODO: set as function
    if (nalType == IDR || nalType == NON_IDR) {

        if (!appendNalToFrame(nalData, nalDataLength, nal->getWidth(), nal->getHeight(), nal->getPresentationTime())) {
            utils::errorMsg("[DashVideoSegmenterHEVC::parseNal] Error appending NAL to frame");
            return false;
        }
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
