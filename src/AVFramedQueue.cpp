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

AVFramedQueue::AVFramedQueue(unsigned maxFrames) : FrameQueue(), max(maxFrames)
{

}

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
        // utils::debugMsg("Forcing front without any frame. Returning NULL pointer");
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

////////////////////////////////////////////
//VIDEO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

VideoFrameQueue* VideoFrameQueue::createNew(VCodecType codec, unsigned maxFrames, PixType pixelFormat)
{
    VideoFrameQueue* q = new VideoFrameQueue(codec, maxFrames, pixelFormat);

    if (!q->setup()) {
        utils::errorMsg("VideoFrameQueue setup error!");
        delete q;
        return NULL;
    }

    return q;
}


VideoFrameQueue::VideoFrameQueue(VCodecType codec_, unsigned maxFrames, PixType pixelFormat_) :
AVFramedQueue(maxFrames), codec(codec_), pixelFormat(pixelFormat_)
{

}

bool VideoFrameQueue::setup()
{
    switch(codec) {
        case H264:
        case H265:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, MAX_H264_OR_5_NAL_SIZE);
            }
            break;
        case VP8:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, LENGTH_VP8);
            }
            break;
        case RAW:
            if (pixelFormat == P_NONE) {
                utils::errorMsg("No pixel fromat defined");
                return false;
                break;
            }
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(codec, DEFAULT_WIDTH, DEFAULT_HEIGHT, pixelFormat);
            }
            break;
        default:
            utils::errorMsg("[Video Frame Queue] Codec not supported!");
            return false;
    }

    return true;
}

////////////////////////////////////////////
//AUDIO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

unsigned getMaxSamples(unsigned sampleRate);

AudioFrameQueue* AudioFrameQueue::createNew(ACodecType codec, unsigned maxFrames, unsigned sampleRate, unsigned channels, SampleFmt sFmt)
{
    AudioFrameQueue* q = new AudioFrameQueue(codec, maxFrames, sFmt, sampleRate, channels);

    if (!q->setup()) {
        utils::errorMsg("AudioFrameQueue setup error!");
        delete q;
        return NULL;
    }

    return q;
}

AudioFrameQueue::AudioFrameQueue(ACodecType codec_, unsigned maxFrames, SampleFmt sFmt, unsigned sampleRate_, unsigned channels_)
: AVFramedQueue(maxFrames), codec(codec_), sampleFormat(sFmt), sampleRate(sampleRate_), channels(channels_)
{

}

bool AudioFrameQueue::setup()
{
    switch(codec) {
        case OPUS:
        case AAC:
        case MP3:
            sampleFormat = S16;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case PCMU:
        case PCM:
            if (sampleFormat == U8 || sampleFormat == S16 || sampleFormat == FLT) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else if (sampleFormat == U8P || sampleFormat == S16P || sampleFormat == FLTP) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else {
                 utils::errorMsg("[Audio Frame Queue] Sample format not supported!");
                 return false;
            }
            break;
        case G711:
            channels = 1;
            sampleRate = 8000;
            sampleFormat = U8;
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        default:
            utils::errorMsg("[Audio Frame Queue] Codec not supported!");
            return false;
    }

    return true;

}
