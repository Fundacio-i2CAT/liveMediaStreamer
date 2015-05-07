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

 class X264or5VideoCircularBuffer : public VideoFrameQueue {

    public:
        X264or5VideoCircularBuffer(VCodecType codec);
        virtual ~X264or5VideoCircularBuffer();

        Frame *getRear();
        void addFrame();
        Frame *forceGetRear();

    protected:
        virtual bool pushBack() = 0;
        virtual Frame* getInputFrame() = 0;

        Frame *innerGetRear();
        Frame *innerForceGetRear();
        bool forcePushBack();
        void innerAddFrame();
        bool setup();

};

#endif
