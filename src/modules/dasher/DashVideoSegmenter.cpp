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

DashVideoSegmenter::DashVideoSegmenter(size_t segDur, std::string segBaseName) : 
DashSegmenter(segDur, MICROSECONDS_TIME_BASE, segBaseName, ".m4v"), 
updatedSPS(false), updatedPPS(false), lastTs(0), tsOffset(0), frameRate(0), isIntra(false), 
isVCL(false), currTimestamp(0), width(0), height(0)
{

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

    currTimestamp = nal->getPresentationTime().count();
    width = nal->getWidth();
    height = nal->getHeight();

    return true;
}

bool DashVideoSegmenter::updateConfig()
{
    if (!updateTimeValues()) {
        utils::errorMsg("Error updating time values of DashVideoSegmenter: timestamp not valid");
        frameData.clear();
        return false;
    }

    if (width <= 0 || height <= 0 || timeBase <= 0 || frameDuration <= 0 || frameRate <= 0) {
        utils::errorMsg("Error configuring DashVideoSegmenter: some config values are not valid");
        frameData.clear();
        return false;
    }

    if(!setup(segmentDuration, timeBase, frameDuration, width, height, frameRate)) {
        utils::errorMsg("Error during Dash Video Segmenter setup");
        frameData.clear();
        return false;
    }

    return true;
}

bool DashVideoSegmenter::finishSegment()
{
    size_t segmentSize = 0;

    if (!dashContext || !dashContext->ctxvideo || dashContext->ctxvideo->segment_data_size <= 0) {
        return true;
    }

    segment->setTimestamp(dashContext->ctxvideo->earliest_presentation_time);
    segmentSize = finish_segment(VIDEO_TYPE, segment->getDataBuffer(), &dashContext);

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }

    segment->setDataLength(segmentSize);

    if(!segment->writeToDisk(getSegmentName())) {
        utils::errorMsg("Error writing DASH segment to disk: invalid path");
        return false;
    }

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

bool DashVideoSegmenter::updateTimeValues() 
{
    if (currTimestamp == 0 || currTimestamp < lastTs) {
        return false;
    }

    if (lastTs <= 0 || tsOffset <= 0 || frameRate <= 0) {
        tsOffset = currTimestamp;
        lastTs = currTimestamp;
        frameRate = VIDEO_DEFAULT_FRAMERATE;
        frameDuration = timeBase/VIDEO_DEFAULT_FRAMERATE;
    } else {
        frameDuration = currTimestamp - lastTs;
        frameRate = timeBase/frameDuration;
        lastTs = currTimestamp;
    }

    return true;
}

bool DashVideoSegmenter::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, VIDEO_TYPE);
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

bool DashVideoSegmenter::generateInitData() 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (!dashContext || metadata.empty()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&metadata[0]);
    dataLength = metadata.size();

    if (!data) {
        return false;
    }

    initSize = new_init_video_handler(data, dataLength, initSegment->getDataBuffer(), &dashContext);

    if (initSize == 0) {
        return false;
    }

    initSegment->setDataLength(initSize);

    return true;
}

bool DashVideoSegmenter::appendFrameToDashSegment()
{
    size_t segmentSize = 0;
    unsigned char* data;
    unsigned dataLength;
    size_t pts;

    if (frameData.empty()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&frameData[0]);
    dataLength = frameData.size();

    if (!data) {
        return false;
    }

    pts = currTimestamp - tsOffset;

    segment->setTimestamp(dashContext->ctxvideo->earliest_presentation_time);
    segmentSize = add_sample(data, dataLength, frameDuration, pts, pts, segment->getSeqNumber(), 
                             VIDEO_TYPE, segment->getDataBuffer(), isIntra, &dashContext);

    frameData.clear();

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }
    
    segment->setDataLength(segmentSize);
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
        frameData.insert(frameData.end(), (nalDataLength >> 24) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 16) & 0xFF);
        frameData.insert(frameData.end(), (nalDataLength >> 8) & 0xFF);
        frameData.insert(frameData.end(), nalDataLength & 0xFF);

        frameData.insert(frameData.end(), nalData, nalData + nalDataLength);
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