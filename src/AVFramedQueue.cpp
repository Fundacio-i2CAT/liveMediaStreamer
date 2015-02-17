/*
 *  AVFramedQueue - A lock-free AV frame circular queue
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *            David Cassany <david.cassany@i2cat.net>
 */

#include "AVFramedQueue.hh"
#include "VideoFrame.hh"
#include "AudioFrame.hh"
#include "Utils.hh"

AVFramedQueue::~AVFramedQueue()
{
    for (unsigned i = 0; i<max; i++) {
        delete frames[i];
    }
}

Frame* AVFramedQueue::getRear() 
{
    if (elements >= max) {
        return NULL;
    }

    return frames[rear];
}

Frame* AVFramedQueue::getFront(bool &newFrame) 
{
    if(frameToRead()) {
        newFrame = true;
        return frames[front];
    }
    newFrame = false;
    return NULL;
}

void AVFramedQueue::addFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
    firstFrame = true;
}

void AVFramedQueue::removeFrame() 
{
    front = (front + 1) % max;
    --elements;
}

void AVFramedQueue::flush() 
{
    if (elements == max) {
        rear = (rear + (max - 1)) % max;
        --elements;
    }
}

Frame* AVFramedQueue::forceGetRear()
{
    Frame *frame;
    while ((frame = getRear()) == NULL) {
        utils::debugMsg("Frame discarted by AVFramedQueue");
        flush();
    }
    return frame;
}

Frame* AVFramedQueue::forceGetFront(bool &newFrame)
{
    if (!firstFrame) {
        utils::debugMsg("Forcing front without any frame. Undefined behaviour");
        return NULL;
    }

    return frames[(front + (max - 1)) % max]; 
}


bool AVFramedQueue::frameToRead()
{
    if (elements <= 0){
        return false;
    } else {
        return true;
    }
}

QueueState AVFramedQueue::getState()
{
    float occupancy = (float)elements/(float)max;
   
    if (occupancy > FAST_THRESHOLD) {
        state = FAST;
    }

    if (occupancy < SLOW_THRESHOLD) {
        state = SLOW;
    }

    return state;
}

////////////////////////////////////////////
//VIDEO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

VideoFrameQueue* VideoFrameQueue::createNew(VCodecType codec, PixType pixelFormat)
{
    return new VideoFrameQueue(codec, pixelFormat);
}

VideoFrameQueue::VideoFrameQueue(VCodecType codec, PixType pixelFormat)
{
    this->codec = codec;
    this->pixelFormat = pixelFormat;

    config();
}

bool VideoFrameQueue::config()
{
    switch(codec) {
        case H264:
            max = DEFAULT_VIDEO_FRAMES;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, LENGTH_H264);
            }
            break;
        case VP8:
            max = DEFAULT_VIDEO_FRAMES;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, LENGTH_VP8);
            }
            break;
        case MJPEG:
            //TODO: implement this initialization
            break;
        case RAW:
            if (pixelFormat == P_NONE) {
                utils::errorMsg("No pixel fromat defined");
                break;
            }
            max = DEFAULT_RAW_FRAMES;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, DEFAULT_WIDTH, DEFAULT_HEIGHT, pixelFormat);
            }
            break;
        default:
            utils::errorMsg("[Video Frame Queue] Codec not supported!");
            return false;
            break;
    }

    return true;
}

////////////////////////////////////////////
//AUDIO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

unsigned getMaxSamples(unsigned sampleRate);

AudioFrameQueue* AudioFrameQueue::createNew(ACodecType codec, unsigned sampleRate, unsigned channels, SampleFmt sFmt)
{
    return new AudioFrameQueue(codec, sFmt, sampleRate, channels);
}

AudioFrameQueue::AudioFrameQueue(ACodecType codec, SampleFmt sFmt, unsigned sampleRate, unsigned channels)
{
    this->codec = codec;
    this->sampleFormat = sFmt;
    this->channels = channels;
    this->sampleRate = sampleRate;

    config();
}

bool AudioFrameQueue::config()
{
    switch(codec) {
        case OPUS:
            max = FRAMES_OPUS;
            sampleFormat = S16;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case AAC:
            max = FRAMES_OPUS;//??
            sampleFormat = S16;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case MP3:
            max = FRAMES_OPUS;
            sampleFormat = S16;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case PCMU:
        case PCM:
            max = FRAMES_AUDIO_RAW;
            if (sampleFormat == U8 || sampleFormat == S16 || sampleFormat == FLT) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else if (sampleFormat == U8P || sampleFormat == S16P || sampleFormat == FLTP) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else {
                //TODO: error
            }
            break;
        case G711:
            channels = 1;
            sampleRate = 8000;
            sampleFormat = U8;
            max = FRAMES_AUDIO_RAW;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        default:
            utils::errorMsg("[Audio Frame Queue] Codec not supported!");
            return false;
            break;
    }

    return true;

}
