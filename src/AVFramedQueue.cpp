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
 */

#include "AVFramedQueue.hh"
#include "VideoFrame.hh"
#include "AudioFrame.hh"
#include <iostream>

Frame* AVFramedQueue::getRear() 
{
    if (elements >= max) {
        return NULL;
    }

    return frames[rear];
}

Frame* AVFramedQueue::getFront() 
{
    if(frameToRead()) {
        return frames[front];
    }
    return NULL;
}

//TODO: add exception if elements > max
void AVFramedQueue::addFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
}

//TODO: add exception if elements < 0
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
        std::cerr << "Frame discarted" << std::endl;
        flush();
    }
    return frame;
}

Frame* AVFramedQueue::forceGetFront()
{
    Frame *frame;
    if ((frame = getFront()) == NULL) {
        std::cerr << "Frame reused" << std::endl;
        frame = getOldie();
    }
    return frame;
}

//TODO shouldn't it be safer?
Frame* AVFramedQueue::getOldie()
{
    return frames[(front + (max - 1)) % max]; 
}

bool AVFramedQueue::frameToRead()
{
    if (elements <= 0){
        return false;
    }
    currentTime = system_clock::now();
    enlapsedTime = duration_cast<milliseconds>(currentTime - frames[front]->getUpdatedTime());
    return enlapsedTime.count() > delay;
}

////////////////////////////////////////////
//VIDEO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

VideoFrameQueue* VideoFrameQueue::createNew(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat)
{
    return new VideoFrameQueue(codec, delay, width, height, pixelFormat);
}

VideoFrameQueue::VideoFrameQueue(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat)
{
    this->codec = codec;
    this->delay = delay;
    this->width = width;
    this->height = height;
    this->pixelFormat = pixelFormat;

    config();
}

bool VideoFrameQueue::config()
{
    switch(codec) {
        case H264:
            max = DEFAULT_VIDEO_FRAMES;
            for (int i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(LENGTH_H264);
            }
            break;
        case VP8:
            //TODO: implement this initialization
            break;
        case MJPEG:
            //TODO: implement this initialization
            break;
        case RAW:
            if (pixelFormat == P_NONE || width == 0 || height == 0) {
                //TODO: error message
                break;
            }
            max = DEFAULT_VIDEO_FRAMES;
            for (int i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(width, height, pixelFormat);
            }
            break;
        default:
            //TODO: error message
            break;
    }
}

////////////////////////////////////////////
//AUDIO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

unsigned getMaxSamples(unsigned sampleRate);

AudioFrameQueue* AudioFrameQueue::createNew(ACodecType codec,  unsigned delay, unsigned sampleRate, unsigned channels, SampleFmt sFmt)
{
    return new AudioFrameQueue(codec, sFmt, sampleRate, channels, delay);
}

AudioFrameQueue::AudioFrameQueue(ACodecType codec, SampleFmt sFmt, unsigned sampleRate, unsigned channels, unsigned delay)
{
    this->codec = codec;
    this->sampleFormat = sFmt;
    this->delay = delay;
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
            for (int i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case PCMU:
        case PCM:
            max = FRAMES_AUDIO_RAW;
            if (sampleFormat == U8 || sampleFormat == S16 || sampleFormat == FLT) {
                for (int i=0; i<max; i++) {
                    frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else if (sampleFormat == U8P || sampleFormat == S16P || sampleFormat == FLTP) {
                for (int i=0; i<max; i++) {
                    frames[i] = PlanarAudioFrame::createNew(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else {
                //TODO: error
            }
        case G711:
            channels = 1;
            sampleRate = 8000;
            sampleFormat = U8;
            max = FRAMES_AUDIO_RAW;
            for (int i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        default:
            //TODO: codec not supported
            break;
    }

}

unsigned getMaxSamples(unsigned sampleRate)
{
    return (AUDIO_FRAME_TIME*sampleRate)/1000;
}
