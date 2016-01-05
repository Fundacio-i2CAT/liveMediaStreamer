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

AVFramedQueue::AVFramedQueue(ConnectionData cData, const StreamInfo *si, unsigned maxFrames) :
        FrameQueue(cData, si), max(maxFrames)
{
    if (max > MAX_FRAMES) {
        utils::errorMsg(std::string("Created an AVFramedQueue with ") + std::to_string(max) + " frames. " +
                "Reducing to " + std::to_string(MAX_FRAMES));
        max = MAX_FRAMES;
    }
    memset(frames, 0, sizeof(frames));
}

AVFramedQueue::~AVFramedQueue()
{
    for (unsigned i = 0; i<max; i++) {
        delete frames[i];
    }
}

Frame* AVFramedQueue::getRear() 
{
    if ((rear + 1) % max == front){
        return NULL;
    }
    
    return frames[rear];
}

Frame* AVFramedQueue::getFront() 
{
    if(rear == front) {
        return NULL;
    }

    return frames[front];
}

//TODO: it should return a vector of filters ID
int AVFramedQueue::addFrame() 
{
    if ((rear + 1) % max == front){
        return connectionData.readers.front().rFilterId;
    }
    rear =  (rear + 1) % max;
    return connectionData.readers.front().rFilterId;
}

int AVFramedQueue::removeFrame() 
{
    if (rear == front){
        return -1;
    }
    front = (front + 1) % max;
    return connectionData.wFilterId;
}

void AVFramedQueue::doFlush() 
{
    rear = (rear + (max - 1)) % max;
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

Frame* AVFramedQueue::forceGetFront()
{
    return frames[(front + (max - 1)) % max]; 
}

unsigned AVFramedQueue::getElements()
{
    return front > rear ? (max - front + rear) : (rear - front);
}


////////////////////////////////////////////
//VIDEO FRAME QUEUE METHODS IMPLEMENTATION//
////////////////////////////////////////////

VideoFrameQueue* VideoFrameQueue::createNew(ConnectionData cData, const StreamInfo *si,
        unsigned maxFrames)
{
    VideoFrameQueue* q = new VideoFrameQueue(cData, si, maxFrames);

    if (!q->setup()) {
        utils::errorMsg("VideoFrameQueue setup error!");
        delete q;
        return NULL;
    }

    return q;
}


VideoFrameQueue::VideoFrameQueue(ConnectionData cData, const StreamInfo *si,
        unsigned maxFrames) : AVFramedQueue(cData, si, maxFrames)
{
}

bool VideoFrameQueue::setup()
{
    switch(streamInfo->video.codec) {
        case H264:
        case H265:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(streamInfo->video.codec, MAX_H264_OR_5_NAL_SIZE);
            }
            break;
        case VP8:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(streamInfo->video.codec, LENGTH_VP8);
            }
            break;
        case RAW:
            if (streamInfo->video.pixelFormat == P_NONE) {
                utils::errorMsg("No pixel fromat defined");
                return false;
                break;
            }
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedVideoFrame::createNew(streamInfo->video.codec,
                        DEFAULT_WIDTH, DEFAULT_HEIGHT, streamInfo->video.pixelFormat);
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

AudioFrameQueue* AudioFrameQueue::createNew(ConnectionData cData, const StreamInfo *si,
        unsigned maxFrames)
{
    AudioFrameQueue* q = new AudioFrameQueue(cData, si, maxFrames);

    if (!q->setup()) {
        utils::errorMsg("AudioFrameQueue setup error!");
        delete q;
        return NULL;
    }

    return q;
}

AudioFrameQueue::AudioFrameQueue(ConnectionData cData, const StreamInfo *si,
        unsigned maxFrames) : AVFramedQueue(cData, si, maxFrames)
{
}

bool AudioFrameQueue::setup()
{
    switch(streamInfo->audio.codec) {
        case OPUS:
        case AAC:
        case MP3:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(streamInfo->audio.channels,
                        streamInfo->audio.sampleRate,
                        AudioFrame::getMaxSamples(streamInfo->audio.sampleRate),
                        streamInfo->audio.codec, streamInfo->audio.sampleFormat);
            }
            break;
        case PCMU:
        case PCM:
            if (streamInfo->audio.sampleFormat == U8 || streamInfo->audio.sampleFormat == S16 || streamInfo->audio.sampleFormat == FLT) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = InterleavedAudioFrame::createNew(
                            streamInfo->audio.channels,
                            streamInfo->audio.sampleRate,
                            AudioFrame::getMaxSamples(streamInfo->audio.sampleRate),
                            streamInfo->audio.codec, streamInfo->audio.sampleFormat);
                }
            } else if (streamInfo->audio.sampleFormat == U8P ||
                       streamInfo->audio.sampleFormat == S16P ||
                       streamInfo->audio.sampleFormat == FLTP) {
                for (unsigned i=0; i<max; i++) {
                    frames[i] = PlanarAudioFrame::createNew(
                            streamInfo->audio.channels,
                            streamInfo->audio.sampleRate,
                            AudioFrame::getMaxSamples(streamInfo->audio.sampleRate),
                            streamInfo->audio.codec, streamInfo->audio.sampleFormat);
                }
            } else {
                 utils::errorMsg("[Audio Frame Queue] Sample format not supported!");
                 return false;
            }
            break;
        case G711:
            for (unsigned i=0; i<max; i++) {
                frames[i] = InterleavedAudioFrame::createNew(
                        streamInfo->audio.channels,
                        streamInfo->audio.sampleRate,
                        AudioFrame::getMaxSamples(streamInfo->audio.sampleRate),
                        streamInfo->audio.codec, streamInfo->audio.sampleFormat);
            }
            break;
        default:
            utils::errorMsg("[Audio Frame Queue] Codec not supported!");
            return false;
    }

    return true;

}
