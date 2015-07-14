/*
 *  DashVideoSegmenter.cpp - DASH video stream segmenter
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *
 */

 #include "DashVideoSegmenter.hh"

DashVideoSegmenter::DashVideoSegmenter(std::chrono::seconds segDur) : 
DashSegmenter(segDur, DASH_VIDEO_TIME_BASE), 
updatedSPS(false), updatedPPS(false), frameRate(0), isIntra(false), 
isVCL(false)
{
    vFrame = InterleavedVideoFrame::createNew(H264, 5000000);
}

DashVideoSegmenter::~DashVideoSegmenter()
{

}

bool DashVideoSegmenter::manageFrame(Frame* frame, bool &newFrame)
{
    VideoFrame* nal;

    nal = dynamic_cast<VideoFrame*>(frame);

    if (!nal) {
        utils::errorMsg("Error managing frame: it MUST be a video frame");
        return false;
    }

    if (!parseNal(nal, newFrame)) {
        utils::errorMsg("Error parsing NAL");
        return false;
    }

    vFrame->setSize(nal->getWidth(), nal->getHeight());
    vFrame->setPresentationTime(nal->getPresentationTime());
    return true;
}

bool DashVideoSegmenter::updateConfig()
{
    return true;
}

bool DashVideoSegmenter::parseNal(VideoFrame* nal, bool &newFrame)
{
    int startCodeOffset;
    unsigned char* nalData;
    int nalDataLength;

    startCodeOffset = detectStartCode(nal->getDataBuf());
    nalData = nal->getDataBuf() + startCodeOffset;
    nalDataLength = nal->getLength() - startCodeOffset;

    if (!nalData || nalDataLength <= 0) {
        utils::errorMsg("Error parsing NAL: invalid data pointer or length");
        return false;
    }

    if (!appendNalToFrame(nalData, nalDataLength, newFrame)) {
        utils::errorMsg("Error parsing NAL: invalid NALU type");
        return false;
    }

    return true;
}

bool DashVideoSegmenter::setup(size_t segmentDuration, size_t timeBase, size_t width, size_t height)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, VIDEO_TYPE);
    }

    if (i2error != I2OK || !dashContext) {
        return false;
    }

    i2error = fill_video_context(&dashContext, width, height, timeBase);

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segmentDuration, &dashContext);
    return true;
}

bool DashVideoSegmenter::updateMetadata()
{
    if (!updatedSPS || !updatedPPS) {
        return false;
    }

    createMetadata();
    updatedSPS = false;
    updatedPPS = false;
    return true;
}

bool DashVideoSegmenter::generateInitData(DashSegment* segment) 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (!setup(segDurInTimeBaseUnits, timeBase, vFrame->getWidth(), vFrame->getHeight())) {
        utils::errorMsg("Error during Dash Video Segmenter setup");
        return false;
    }

    if (!dashContext || metadata.empty() || !segment || !segment->getDataBuffer()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&metadata[0]);
    dataLength = metadata.size();

    if (!data) {
        return false;
    }

    initSize = new_init_video_handler(data, dataLength, segment->getDataBuffer(), &dashContext);

    if (initSize == 0) {
        return false;
    }

    segment->setDataLength(initSize);

    return true;
}

bool DashVideoSegmenter::appendFrameToDashSegment(DashSegment* segment)
{
    size_t addSampleReturn;
    size_t timeBasePts;

    if (!vFrame->getDataBuf() || vFrame->getLength() <= 0 || !dashContext) {
        utils::errorMsg("Error appeding frame to segment: frame not valid");
        return false;
    }

    timeBasePts = microsToTimeBase(vFrame->getPresentationTime());

    addSampleReturn = add_video_sample(vFrame->getDataBuf(), vFrame->getLength(), timeBasePts, 
                                        timeBasePts, segment->getSeqNumber(), isIntra, &dashContext);

    vFrame->setLength(0);

    if (addSampleReturn != I2OK) {
        utils::errorMsg("Error adding video sample. Code error: " + std::to_string(addSampleReturn));
        return false;
    }

    return true;
}

bool DashVideoSegmenter::generateSegment(DashSegment* segment)
{
    size_t segmentSize = 0;
    uint32_t segTimestamp;
    uint32_t segDuration;
    size_t timeBasePts;

    timeBasePts = microsToTimeBase(vFrame->getPresentationTime());
    segmentSize = generate_video_segment(isIntra, timeBasePts, segment->getDataBuffer(), &dashContext, &segTimestamp, &segDuration);


    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }

    segment->setTimestamp(segTimestamp);
    segment->setDuration(segDuration);
    segment->setDataLength(segmentSize);
    return true;
}

bool DashVideoSegmenter::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, VIDEO_TYPE);
    return true;
}

bool DashVideoSegmenter::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, bool &newFrame)
{
    unsigned char nalType;

    nalType = nalData[0] & H264_NALU_TYPE_MASK;

    switch (nalType) {
        case SPS:
            saveSPS(nalData, nalDataLength);
            isVCL = false;
            isIntra = false;
            newFrame = false;
            break;
        case PPS:
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
        case AUD:
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
        vFrame->getDataBuf()[vFrame->getLength()] = (nalDataLength >> 24) & 0xFF;
        vFrame->getDataBuf()[vFrame->getLength()+1] = (nalDataLength >> 16) & 0xFF;
        vFrame->getDataBuf()[vFrame->getLength()+2] = (nalDataLength >> 8) & 0xFF;
        vFrame->getDataBuf()[vFrame->getLength()+3] = nalDataLength & 0xFF;
        vFrame->setLength(vFrame->getLength() + 4);

        memcpy(vFrame->getDataBuf() + vFrame->getLength(), nalData, nalDataLength);
        vFrame->setLength(vFrame->getLength() + nalDataLength);
    }

    return true;
}


void DashVideoSegmenter::saveSPS(unsigned char* data, int dataLength)
{
    lastSPS.clear();
    lastSPS.insert(lastSPS.begin(), data, data + dataLength);
    updatedSPS = true;
}

void DashVideoSegmenter::savePPS(unsigned char* data, int dataLength)
{
    lastPPS.clear();
    lastPPS.insert(lastPPS.begin(), data, data + dataLength);
    updatedPPS = true;
}

void DashVideoSegmenter::createMetadata()
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

int DashVideoSegmenter::detectStartCode(unsigned char const* ptr)
{
    u_int32_t bytes = 0|(ptr[0]<<16)|(ptr[1]<<8)|ptr[2];
    if (bytes == H264_NALU_START_CODE) {
        return SHORT_START_CODE_LENGTH;
    }

    bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    if (bytes == H264_NALU_START_CODE) {
        return LONG_START_CODE_LENGTH;
    }
    return 0;
}
