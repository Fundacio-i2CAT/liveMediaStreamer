/*
 *  SlicedVideoFrameQueue - Video circular buffer for x264 and x265
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

#ifndef _SLICED_VIDEO_FRAME_QUEUE_HH
#define _SLICED_VIDEO_FRAME_QUEUE_HH

#include "Types.hh"
#include "AVFramedQueue.hh"
#include "VideoFrame.hh"

/*! Virtual interface for X264VideoCircularBuffer and X265VideoCircularBuffer. In is a child class from VideoFrameQueue, modifying its 
    input behaviour. */

class SlicedVideoFrameQueue : public VideoFrameQueue {

public:
    /**
    * Class constructor wrapper
    * @param maxFrames internal frame queue size 
    * @return NULL if wrong input parameters or wrong init and pointer to new object if success
    */
    static SlicedVideoFrameQueue* createNew(int wId, int rId, VCodecType codec, unsigned maxFrames, unsigned maxSliceSize);

    /**
    * Class destructor
    */
    virtual ~SlicedVideoFrameQueue();

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
    int addFrame();

    /**
    * It returns the input frame, flushing the internal buffer if the internal buffer is full. It may cause data loss.
    * @return input frame pointer
    */
    Frame *forceGetRear();

private:
    SlicedVideoFrameQueue(int wId, int rId, VCodecType codec, unsigned maxFrames);

    void pushBackSliceGroup(Slice* slices, int sliceNum);
    Frame *innerGetRear();
    Frame *innerForceGetRear();
    void innerAddFrame();
    bool setup(unsigned maxSliceSize);

    SlicedVideoFrame* inputFrame;

};

#endif
