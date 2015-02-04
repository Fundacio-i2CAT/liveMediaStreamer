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

Reader* Dasher::setReader(int readerID, FrameQueue* queue, bool sharedQueue = false)
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

DashSegmenter::DashSegmenter() : dashContext(NULL)
{

}

DashVideoSegmenter::DashVideoSegmenter() : 
DashSegmenter() 
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

    if (!setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, vFrame->getWidth(), vFrame->getHeight(), size_t framerate)) {
        utils::errorMsg("Error while setupping DashVideoSegmenter");
        return false;
    }

    
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
}

bool DashAudioSegmenter::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, AUDIO_TYPE);
    }
    
    if (i2error != I2OK || !dashContext) {
        return false;
    }

    i2error = fill_audio_context(&dashContext, channels, sampleRate, sampleSize, timeBase, sampleDuration); 

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segmentDuration, &dashContext);
    return true;
}

