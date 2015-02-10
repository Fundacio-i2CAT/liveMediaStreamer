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
DashSegmenter(segDur, MICROSECONDS_TIME_BASE, segBaseName),
updatedSPS(false), updatedPPS(false), lastTs(0), tsOffset(0), frameRate(0), isIntra(false)
{

}

DashVideoSegmenter::~DashVideoSegmenter() 
{

}

bool DashVideoSegmenter::manageFrame(Frame* frame)
{
    VideoFrame* vFrame;
    bool newFrame;
    unsigned int ptsMinusOffset;

    vFrame = dynamic_cast<VideoFrame*>(frame);

    if (!vFrame) {
        utils::errorMsg("Error managing frame: it MUST be a video frame");
        return false;
    }

    newFrame = parseNal(vFrame);

    if (!newFrame) {
        return true;
    }

    if (!updateTimeValues(vFrame->getPresentationTime().count())) {
        frameData.clear();
        return true;
    }

    if(!setup(segmentDuration, timeBase, frameDuration, vFrame->getWidth(), vFrame->getHeight(), frameRate)) {
        utils::errorMsg("Error during Dash Video Segmenter setup");
        frameData.clear();
        return false;
    }

    if (updateMetadata() && generateInit()) {
        
        if(!initSegment->writeToDisk(getInitSegmentName())) {
            utils::errorMsg("Error writing DASH init segment to disk: invalid path");
            frameData.clear();
            return false;
        }
    }

    ptsMinusOffset = vFrame->getPresentationTime().count() - tsOffset;
    if (appendFrameToDashSegment(ptsMinusOffset)) {
        if(!segment->writeToDisk(getSegmentName())) {
            utils::errorMsg("Error writing DASH segment to disk: invalid path");
            frameData.clear();
            return false;
        }

        segment->incrSeqNumber();
    }

    frameData.clear();
    return true;
    
}

bool DashVideoSegmenter::finishSegment()
{
    
}


bool DashVideoSegmenter::parseNal(VideoFrame* nal) 
{
    bool newFrame;
    int startCodeOffset;
    unsigned char* nalData;
    int nalDataLength;

    startCodeOffset = detectStartCode(nal->getDataBuf());
    nalData = nal->getDataBuf() + startCodeOffset;
    nalDataLength = nal->getLength() - startCodeOffset;
    newFrame = appendNalToFrame(nalData, nalDataLength);

    return newFrame;
}

bool DashVideoSegmenter::updateTimeValues(size_t currentTimestamp) 
{
    if (lastTs <= 0 || tsOffset <= 0 || timeBase <= 0) {
        tsOffset = currentTimestamp;
        lastTs = currentTimestamp;
        return false;
    }

    frameDuration = currentTimestamp - lastTs;
    frameRate = timeBase/frameDuration;
    lastTs = currentTimestamp;
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

bool DashVideoSegmenter::generateInit() 
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

bool DashVideoSegmenter::appendFrameToDashSegment(size_t pts)
{
    size_t segmentSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (frameData.empty()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&frameData[0]);
    dataLength = frameData.size();

    if (!data) {
        return false;
    }

    segment->setTimestamp(dashContext->ctxvideo->earliest_presentation_time);
    segmentSize = add_sample(data, dataLength, frameDuration, pts, pts, segment->getSeqNumber(), 
                             VIDEO_TYPE, segment->getDataBuffer(), isIntra, &dashContext);

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }

    
    segment->setDataLength(segmentSize);
    return true;
}

std::string DashVideoSegmenter::getSegmentName()
{
    std::string fullName;

    fullName = baseName + "_" + std::to_string(segment->getTimestamp()) + ".m4v";
    
    return fullName;
}

std::string DashVideoSegmenter::getInitSegmentName()
{
    std::string fullName;

    fullName = baseName + "_init.m4v";
    
    return fullName;
}

bool DashVideoSegmenter::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength)
{
    unsigned char nalType; 

    nalType = nalData[0] & H264_NALU_TYPE_MASK;

    if (nalType == SPS) {
        saveSPS(nalData, nalDataLength);
        return false;
    }

    if (nalType == PPS) {
        savePPS(nalData, nalDataLength);
        return false;
    }

    if (nalType == SEI) {
        return false;
    }

    if (nalType == AUD) {
        return true;
    }

    isIntra = (nalType == IDR);

    frameData.insert(frameData.end(), (nalDataLength >> 24) & 0xFF);
    frameData.insert(frameData.end(), (nalDataLength >> 16) & 0xFF);
    frameData.insert(frameData.end(), (nalDataLength >> 8) & 0xFF);
    frameData.insert(frameData.end(), nalDataLength & 0xFF);

    frameData.insert(frameData.end(), nalData, nalData + nalDataLength);

    return false;
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