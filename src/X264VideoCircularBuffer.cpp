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
 *            David Cassany <david.cassany@i2cat.net>
 */

#include "X264VideoCircularBuffer.hh"
#include <cstring>
#include <string>
#include <sys/time.h>
#include "Utils.hh"

X264VideoCircularBuffer* X264VideoCircularBuffer::createNew()
{
    return new X264VideoCircularBuffer();
}

X264VideoCircularBuffer::~X264VideoCircularBuffer()
{
    //TODO: implement destructor
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


X264VideoCircularBuffer::X264VideoCircularBuffer(): VideoFrameQueue(H264, YUYV422)
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
    int nalsNum;
    x264_nal_t** nals;
    unsigned char** hNals;
    int* hNalSize;
    
    if ((nalsNum = inputFrame->getHeaderNalsNum()) > 0){       
        hNals = inputFrame->getHeaderNals();
        hNalSize = inputFrame->getHeaderNalsSize();
        
        for (int i=0; i<nalsNum; i++) {
            if ((interleavedVideoFrame = innerGetRear()) == NULL){
                interleavedVideoFrame = innerForceGetRear();
            }
            
            memcpy(interleavedVideoFrame->getDataBuf(), hNals[i], hNalSize[i]);
            interleavedVideoFrame->setLength(hNalSize[i]);
            interleavedVideoFrame->setPresentationTime(inputFrame->getPresentationTime());
            innerAddFrame();
        }
    }
    
    nalsNum = inputFrame->getNalsNum();
    nals = inputFrame->getNals();

    for (int i=0; i<nalsNum; i++) {
        if ((interleavedVideoFrame = innerGetRear()) == NULL){
            interleavedVideoFrame = innerForceGetRear();
        }

        memcpy(interleavedVideoFrame->getDataBuf(), (*nals)[i].p_payload, (*nals)[i].i_payload);
        interleavedVideoFrame->setLength((*nals)[i].i_payload);
		interleavedVideoFrame->setPresentationTime(inputFrame->getPresentationTime());
		innerAddFrame();
	}
	
	inputFrame->clearNals();
	
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
        utils::debugMsg("Frame discarted by X264 Circular Buffer");
        flush();
    }
    return frame;
}

void X264VideoCircularBuffer::innerAddFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
}
