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

#include "X264VideoCircularBuffer.hh"
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include "Utils.hh"

X264VideoCircularBuffer* X264VideoCircularBuffer::createNew()
{
    return new X264VideoCircularBuffer();
}


Frame* X264VideoCircularBuffer::getRear()
{
    if (elements >= max) {
        return NULL;
    }
    
    return inputFrame;
}

void X264VideoCircularBuffer::addFrame()
{
    forcePushBack();
}

Frame* X264VideoCircularBuffer::forceGetRear()
{
    return inputFrame;
}


X264VideoCircularBuffer::X264VideoCircularBuffer(): VideoFrameQueue(H264, 0, YUYV422)
{
    config();
}

bool X264VideoCircularBuffer::config()
{
    inputFrame = X264VideoFrame::createNew(codec, DEFAULT_WIDTH, DEFAULT_HEIGHT, pixelFormat);
    return true;
}

bool X264VideoCircularBuffer::pushBack() 
{
    Frame* interleavedVideoFrame;
    
	int sizeBuffer = inputFrame->getSizeNals();
	x264_nal_t** buffer = inputFrame->getNals();

    int i = 0;
    for (i=0; i<sizeBuffer; i++) {
        int sizeNal = (*buffer)[i].i_payload;
        if ((interleavedVideoFrame = innerGetRear()) == NULL){
            interleavedVideoFrame = innerForceGetRear();
        }

		memcpy(interleavedVideoFrame->getDataBuf(), (*buffer)[i].p_payload, sizeNal);
		interleavedVideoFrame->setLength(sizeNal);
		interleavedVideoFrame->setPresentationTime(inputFrame->getPresentationTime());
		innerAddFrame();
	}
	
    return true;
}   

bool X264VideoCircularBuffer::forcePushBack()
{
    return pushBack();
}

Frame* X264VideoCircularBuffer::innerGetRear() 
{
    if (elements >= max) {
        return NULL;
    }
    
    return frames[rear];
}

Frame* X264VideoCircularBuffer::innerForceGetRear()
{
    Frame *frame;
    while ((frame = innerGetRear()) == NULL) {
        utils::debugMsg("Frame discarted");
        flush();
    }
    return frame;
}

void X264VideoCircularBuffer::innerAddFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
}
