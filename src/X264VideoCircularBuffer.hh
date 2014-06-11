/*
 *  X264VideoCircularBuffer - Video circular buffer
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:  Martin German <martin.german@i2cat.net>
 */

#ifndef _VIDEO_CIRCULAR_BUFFER_HH
#define _VIDEO_CIRCULAR_BUFFER_HH

#define MAX_NALS 100

#include <atomic>
#include "Types.hh"
#include "AVFramedQueue.hh"
#include "X264VideoFrame.hh"

extern "C" {
	#include <x264.h>
}

 class X264VideoCircularBuffer : public VideoFrameQueue {

    public:
        static X264VideoCircularBuffer* createNew();
        ~X264VideoCircularBuffer();

        Frame *getRear();
        void addFrame();
        Frame *forceGetRear();

    protected:
        bool config();


    private:
        X264VideoCircularBuffer();
        bool frameToRead() {return moreNals;};

        bool pushBack();
        bool popFront();
        bool forcePushBack();
        
		bool moreNals;

        X264VideoFrame* inputFrame;
};

#endif
