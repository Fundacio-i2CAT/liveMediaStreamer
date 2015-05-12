/*
 *  X264or5VideoCircularBuffer - Video circular buffer for x264 and x265
 *  Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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

#ifndef _X264_OR_5_VIDEO_CIRCULAR_BUFFER_HH
#define _X264_OR_5_VIDEO_CIRCULAR_BUFFER_HH

#include "Types.hh"
#include "AVFramedQueue.hh"

/*! Virtual interface for X264VideoCircularBuffer and X265VideoCircularBuffer. In is a child class from VideoFrameQueue, modifying its 
    input behaviour. */

class X264or5VideoCircularBuffer : public VideoFrameQueue {

public:
    /**
    * Class destructor
    */
    virtual ~X264or5VideoCircularBuffer();

    /**
    * It returns the input frame
    * @return input frame pointer or NULL if internal buffer is full
    */
    Frame *getRear();

    /**
    * It dumps input frame data into internal VideoFrameQueue. Each NAL unit stored in input frame is copied
    * into a VideoFrame structure.
    * @return input frame pointer or NULL if internal buffer is full
    */
    void addFrame();

    /**
    * It returns the input frame, flushing the internal buffer if the internal buffer is full. It may cause data loss.
    * @return input frame pointer
    */
    Frame *forceGetRear();

protected:
    X264or5VideoCircularBuffer(VCodecType codec, unsigned maxFrames);

    virtual bool pushBack() = 0;

    Frame *innerGetRear();
    Frame *innerForceGetRear();
    void innerAddFrame();
    bool setup();

    Frame* inputFrame;

};

#endif
