/*
 *  Dasher.cpp - Class that handles DASH sessions
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *            
 */
 
#include "Dasher.hh"
#include "../../AVFramedQueue.hh"
#include "../../VideoFrame.hh"
#include "../../AudioFrame.hh"

#include <map>
#include <string>
#include <chrono>
#include <fstream>

Dasher::Dasher(int segDur, int readersNum) : 
TailFilter(readersNum), segmentDuration(segDur)
{

}

Dasher::~Dasher()
{

}

bool Dasher::doProcessFrame(std::map<int, Frame*> orgFrames)
{
    for (auto fr : orgFrames) {

        if (!fr.second) {
            //TODO: check this
            return false;
        }

        segmenters[fr.first]->manageFrame(fr.second);
    }

    return true;
}

void Dasher::initializeEventMap()
{

}

void Dasher::doGetState(Jzon::Object &filterNode)
{
//TODO: implement    
}

Reader* Dasher::setReader(int readerId, FrameQueue* queue, bool sharedQueue)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    
    if (readers.size() >= getMaxReaders() || readers.count(readerId) > 0 ) {
        return NULL;
    }
    
    Reader* r = new Reader();
    readers[readerId] = r;
    
    if ((vQueue = dynamic_cast<VideoFrameQueue*>(queue)) != NULL) {

        if (vQueue->getCodec() != H264) {
            utils::errorMsg("Error setting dasher reader: only H264 codec is supported for video");
            return NULL;
        }

        segmenters[readerId] = new DashVideoSegmenter(segmentDuration);
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(queue)) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return NULL;
        }

        segmenters[readerId] = new DashAudioSegmenter(segmentDuration);
    }
    
    return r;

}

DashSegmenter::DashSegmenter(size_t segDur) : 
dashContext(NULL), timeBase(MICROSECONDS_TIME_BASE), segmentDuration(segDur), frameDuration(0)
{
    segment = new DashSegment(MAX_DAT);
    initSegment = new DashSegment(MAX_DAT);
    baseName = "/home/palau/nginx_root/dashLMS/test500";
}

DashVideoSegmenter::DashVideoSegmenter(size_t segDur) : DashSegmenter(segDur),
updatedSPS(false), updatedPPS(false), lastTs(0), tsOffset(0), frameRate(0), isIntra(false)
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

    segmentSize = add_sample(data, dataLength, frameDuration, pts, pts, segment->getSeqNumber(), 
                             VIDEO_TYPE, segment->getDataBuffer(), isIntra, &dashContext);

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }

    segment->setTimestamp(dashContext->ctxvideo->earliest_presentation_time);
    segment->setDataLength(segmentSize);
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


DashAudioSegmenter::DashAudioSegmenter(size_t segDur) : 
DashSegmenter(segDur)
{

}

bool DashAudioSegmenter::manageFrame(Frame* frame)
{
    AudioFrame* aFrame;

    aFrame = dynamic_cast<AudioFrame*>(frame);

    if (!aFrame) {
        utils::errorMsg("Error managing frame: it MUST be an audio frame");
        return false;
    }

    for(int i = 0; i < aFrame->getLength(); i++) {
        printf("%x ", aFrame->getDataBuf()[i]);
    }
    printf("\n\n");

    return true;
}

bool DashAudioSegmenter::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample)
{
    // uint8_t i2error = I2OK;

    // if (!dashContext) {
    //     i2error = generate_context(&dashContext, AUDIO_TYPE);
    // }
    
    // if (i2error != I2OK || !dashContext) {
    //     return false;
    // }

    // i2error = fill_audio_context(&dashContext, channels, sampleRate, sampleSize, timeBase, sampleDuration); 

    // if (i2error != I2OK) {
    //     return false;
    // }

    // set_segment_duration(segmentDuration, &dashContext);
    return true;
}

DashSegment::DashSegment(size_t maxSize)
{
    data = new unsigned char[maxSize];
    timestamp = 0;
    dataLength = 0;
    seqNumber = 0;
}

DashSegment::~DashSegment()
{
    delete[] data;
}

void DashSegment::setSeqNumber(size_t seqNum)
{
    seqNumber = seqNum;
}

void DashSegment::setDataLength(size_t length)
{
    dataLength = length;
}

bool DashSegment::writeToDisk(std::string path)
{
    const char* p = path.c_str();
    std::ofstream file(p, std::ofstream::binary);

    if (!file) {
        return false;
    }

    file.write((char*)data, dataLength);
    file.close();
    return true;
}

void DashSegment::setTimestamp(size_t ts)
{
    timestamp = ts;
}

void DashSegment::clear()
{
    timestamp = 0;
    dataLength = 0;
    seqNumber = 0;
}

