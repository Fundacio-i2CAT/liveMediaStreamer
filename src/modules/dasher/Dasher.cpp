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

Dasher::Dasher(int readersNum) : 
TailFilter(readersNum) 
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

        segmenters[readerId] = new DashVideoSegmenter();
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(queue)) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return NULL;
        }

        segmenters[readerId] = new DashAudioSegmenter();
    }
    
    return r;

}

DashSegmenter::DashSegmenter()// : dashContext(NULL)
{

}

DashVideoSegmenter::DashVideoSegmenter() : updatedSPS(false), updatedPPS(false)
{

}

bool DashVideoSegmenter::manageFrame(Frame* frame)
{
    VideoFrame* vFrame;

    vFrame = dynamic_cast<VideoFrame*>(frame);

    if (!vFrame) {
        utils::errorMsg("Error managing frame: it MUST be a video frame");
        return false;
    }

    parseNal(vFrame);

    // if (!setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, vFrame->getWidth(), vFrame->getHeight(), size_t framerate)) {
    //     utils::errorMsg("Error while setupping DashVideoSegmenter");
    //     return false;
    // }
    return true;
    
}

bool DashVideoSegmenter::parseNal(VideoFrame* nal) 
{
    int startCodeOffset = detectStartCode(nal->getDataBuf());
    unsigned char* nalData = nal->getDataBuf() + startCodeOffset;
    int nalDataLength = nal->getLength() - startCodeOffset;
    bool newFrame;

    
    newFrame = appendNalToFrame(nalData, nalDataLength);

    if (newFrame) {
        
        frameData.clear();
    }

    if (updateMetadata()) {
        //generateInit()
    }


    return true;
}

bool DashVideoSegmenter::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength)
{
    unsigned char nalType; 
    bool completeFrame = false;

    nalType = nalData[0] & H264_NALU_TYPE_MASK;

    switch(nalType) {
        case SPS:
            saveSPS(nalData, nalDataLength);
            break;
        case PPS:
            savePPS(nalData, nalDataLength);
            break;
        case SEI:
            break;
        case AUD:
            completeFrame = true;
            break;
        case IDR:
            break;
        default:
            break;
    }

    frameData.insert(frameData.end(), (nalDataLength >> 24) & 0xFF);
    frameData.insert(frameData.end(), (nalDataLength >> 16) & 0xFF);
    frameData.insert(frameData.end(), (nalDataLength >> 8) & 0xFF);
    frameData.insert(frameData.end(), nalDataLength & 0xFF);

    frameData.insert(frameData.end(), nalData, nalData + nalDataLength);

    return completeFrame;
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
    if (updatedSPS && updatedPPS) {
        createMetadata();
        updatedSPS = false;
        updatedPPS = false;
        return true;
    }

    return false;
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
    // uint8_t i2error = I2OK;

    // if (!dashContext) {
    //     i2error = generate_context(&dashContext, VIDEO_TYPE);
    // }

    // if (i2error != I2OK || !dashContext) {
    //     return false;
    // }

    // i2error = fill_video_context(&dashContext, width, height, framerate, timeBase, sampleDuration);

    // if (i2error != I2OK) {
    //     return false;
    // }

    // set_segment_duration(segmentDuration, &dashContext);
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


DashAudioSegmenter::DashAudioSegmenter() : 
DashSegmenter()
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

