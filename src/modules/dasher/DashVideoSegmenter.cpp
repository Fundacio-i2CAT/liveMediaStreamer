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
 *            Gerard Castillo <gerard.castillo@i2cat.net>  
 *
 */

 #include "DashVideoSegmenter.hh"

DashVideoSegmenter::DashVideoSegmenter(std::chrono::seconds segDur, std::string video_format_) : 
DashSegmenter(segDur, DASH_VIDEO_TIME_BASE), 
frameRate(0), isIntra(false), 
width(0), height(0),
video_format(video_format_)
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

    currDuration = nal->getDuration();
    currTimestamp = nal->getPresentationTime(); 
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

    if(!setup(segDurInTimeBaseUnits, timeBase, frameDuration, width, height, frameRate)) {
        utils::errorMsg("Error during Dash Video Segmenter setup");
        frameData.clear();
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
    if (currDuration.count() == 0) {
        return false;
    }

    frameDuration = nanosToTimeBase(currDuration);
    frameRate = std::nano::den/currDuration.count();
    return true;
}

bool DashVideoSegmenter::generateInitData(DashSegment* segment) 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

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
    unsigned char* data;
    unsigned dataLength;
    size_t addSampleReturn;

    if (frameData.empty()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&frameData[0]);
    dataLength = frameData.size();

    if (!data) {
        return false;
    }

    theoricPts = customTimestamp(currTimestamp);

    addSampleReturn = add_video_sample(data, dataLength, frameDuration, theoricPts, theoricPts, segment->getSeqNumber(), isIntra, &dashContext);
    frameData.clear();

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

    if (frameData.empty()) {
        return false;
    }

    segmentSize = generate_video_segment(isIntra, segment->getDataBuffer(), &dashContext, &segTimestamp, &segDuration);

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }
    
    segment->setTimestamp(segTimestamp);
    segment->setDuration(segDuration);
    segment->setDataLength(segmentSize);
    return true;
}

int DashVideoSegmenter::detectStartCode(unsigned char const* ptr)
{
    u_int32_t bytes = 0|(ptr[0]<<16)|(ptr[1]<<8)|ptr[2];
    if (bytes == H264or5_NALU_START_CODE) {
        return SHORT_START_CODE_LENGTH;
    }

    bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    if (bytes == H264or5_NALU_START_CODE) {
        return LONG_START_CODE_LENGTH;
    }
    return 0;
}
