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

#ifndef _AV_FRAMED_QUEUE_HH
#define _AV_FRAMED_QUEUE_HH

#include "FrameQueue.hh"

class AVFramedQueue : public FrameQueue {

public:
    virtual Frame *getRear();
    Frame *getFront();
    virtual void addFrame();
    void removeFrame();
    void flush();
    virtual Frame *forceGetRear();
    Frame *forceGetFront();
    int delay; //(ms)
    const int getElements() {return elements;};
    bool frameToRead();

protected:

    Frame* frames[MAX_FRAMES];
    std::atomic<int> elements;
    int max;
    
};

class VideoFrameQueue : public AVFramedQueue {

public:
    static VideoFrameQueue* createNew(VCodecType codec, unsigned delay, unsigned width = 0, 
                                            unsigned height = 0, PixType pixelFormat = P_NONE);
    
    const VCodecType getCodec() const {return codec;};

protected:
    VideoFrameQueue(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat);
    VCodecType codec;
    PixType pixelFormat;
    unsigned width;
    unsigned height;
    bool config();

};

class AudioFrameQueue : public AVFramedQueue {

public:
    static AudioFrameQueue* createNew(ACodecType codec, unsigned delay, unsigned sampleRate = 48000, unsigned channels = 2, SampleFmt sFmt = S16);
    
    unsigned getSampleRate() const {return sampleRate;};
    unsigned getChannels() const {return channels;};
    const ACodecType getCodec() const {return codec;};
    const SampleFmt getSampleFmt() const {return sampleFormat;};

protected:
    AudioFrameQueue(ACodecType codec, SampleFmt sFmt, unsigned sampleRate, unsigned channels, unsigned delay);

    ACodecType codec;
    SampleFmt sampleFormat;
    unsigned sampleRate;
    unsigned channels;
    bool config();

};

#endif
