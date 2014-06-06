/*
 *  VideoCircularBuffer - Video circular buffer
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
#include "FrameQueue.hh"
#include "X264VideoFrame.hh"

extern "C" {
	#include <x264.h>
}

 class VideoCircularBuffer : public FrameQueue {

    public:
        static VideoCircularBuffer* createNew();
        ~VideoCircularBuffer();

        Frame *getRear();
        Frame *getFront();
        void addFrame();
        void removeFrame();
        void flush();
        Frame *forceGetRear();
        Frame *forceGetFront();
        int delay; //(ms)
        bool frameToRead() {return moreNals};

    protected:
        bool config();


    private:
        VideoCircularBuffer();

        bool pushBack(x264_nal_t **buffer, unsigned int sizeBuffer);
        bool popFront(unsigned char **buffer, unsigned int *length);
        bool forcePushBack(x264_nal_t **buffer, unsigned int sizeBuffer);
        
        unsigned int first;
		unsigned int last;
		unsigned char *data[MAX_NALS+1];
		unsigned int length[MAX_NALS+1];
        bool moreNals;

        X264VideoFrame* inputFrame;
        InterleavedVideoFrame* outputFrame;
};

#endif
