/*
 *  X264or5VideoCircularBuffer - Video circular buffer for x264 and x265
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#include "X264or5VideoCircularBuffer.hh"
#include "Utils.hh"

X264or5VideoCircularBuffer::X264or5VideoCircularBuffer(VCodecType codec) : VideoFrameQueue(codec)
{
    config();
}

X264or5VideoCircularBuffer::~X264or5VideoCircularBuffer()
{
    //TODO: implement destructor
}

Frame* X264or5VideoCircularBuffer::getRear()
{
    if (elements >= max) {
        return NULL;
    }
    
    return inputFrame;
}

void X264or5VideoCircularBuffer::addFrame()
{
    forcePushBack();
}

Frame* X264or5VideoCircularBuffer::forceGetRear()
{
    return inputFrame;
}

bool X264or5VideoCircularBuffer::forcePushBack()
{
    return pushBack();
}

Frame* X264or5VideoCircularBuffer::innerGetRear() 
{
    if (elements >= max) {
        return NULL;
    }
    return frames[rear];
}

Frame* X264or5VideoCircularBuffer::innerForceGetRear()
{
    Frame *frame;
    while ((frame = innerGetRear()) == NULL) {
        utils::debugMsg("Frame discarted by X264 Circular Buffer");
        flush();
    }
    return frame;
}

void X264or5VideoCircularBuffer::innerAddFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
}