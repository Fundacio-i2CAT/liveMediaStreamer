/*
 *  X264VideoCircularBuffer - X264 Video circular buffer
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
 *            David Cassany <david.cassany@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _X264_VIDEO_CIRCULAR_BUFFER_HH
#define _X264_VIDEO_CIRCULAR_BUFFER_HH

#include "Types.hh"
#include "X264or5VideoCircularBuffer.hh"
#include "X264VideoFrame.hh"

/*! Video circular buffer, which has an X264VideoFrame as input interface and dumps each 
    NAL unit into an internal VideoFrameQueue, which is its output interface */

class X264VideoCircularBuffer : public X264or5VideoCircularBuffer {

    public:
        /**
        * Class constructor wrapper
        * @param maxFrames internal frame queue size 
        * @return NULL if wrong input parameters or wrong init and pointer to new object if success
        */
        static X264VideoCircularBuffer* createNew(unsigned maxFrames);

        /**
        * Class destructor
        */
        ~X264VideoCircularBuffer();

    private:
        X264VideoCircularBuffer(unsigned maxFrames);
        bool pushBack();
};

#endif
