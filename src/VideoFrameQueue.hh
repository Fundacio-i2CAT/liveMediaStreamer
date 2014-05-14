/*
 *  VideoFrameQueue - A lock-free video frame circular queue
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

#ifndef _VIDEO_FRAME_QUEUE_HH
#define _VIDEO_FRAME_QUEUE_HH

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#include "Types.hh"

class VideoFrameQueue : public FrameQueue {

public:
    static VideoFrameQueue* createNew(VCodecType codec, unsigned delay, unsigned width = 0, 
                                            unsigned height = 0, PixType pixelFormat = P_NONE);

protected:
    VideoFrameQueue(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat);
    VCodecType codec;
    PixType pixelFormat;
    unsigned width;
    unsigned height;
    bool config();

};

#endif