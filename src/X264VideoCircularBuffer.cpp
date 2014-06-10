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

X264VideoCircularBuffer* X264VideoCircularBuffer::createNew()
{
    return new X264VideoCircularBuffer();
}


Frame* X264VideoCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* X264VideoCircularBuffer::getFront()
{
    if (moreNals == false) {
        return NULL;
    }
	
    if (!popFront() {
		moreNals = false;
        //std::cerr << "There is not enough data to fill a frame. Impossible to get frame!\n";
        return NULL;
    }

	moreNals = true;    
    return outputFrame;
}

void X264VideoCircularBuffer::addFrame()
{
    forcePushBack();
	moreNals = true;
}

void X264VideoCircularBuffer::removeFrame()
{
    //TODO
}

void X264VideoCircularBuffer::flush() 
{
    return;
}


Frame* X264VideoCircularBuffer::forceGetRear()
{
    return getRear();
}

Frame* X264VideoCircularBuffer::forceGetFront()
{
    if (!popFront()) {
        std::cerr << "There is not enough data to fill a frame. Reusing previous frame!\n";
		return NULL;
    }

    return outputFrame;
}

X264VideoCircularBuffer::X264VideoCircularBuffer()
{
	first = last = 0;    
    moreNals = false;

    config();
}

bool X264VideoCircularBuffer::config()
{
    inputFrame = X264VideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);
    outputFrame = InterleavedVideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);

    return true;
}

X264VideoCircularBuffer::~X264VideoCircularBuffer()
{
	int i = 0, count = (last - first);
	if (count < 0)
		count = last + ((MAX_NALS + 1) - first);
	for (i=0; i < count; i++)
		delete[] data[((first + i) % (MAX_NALS + 1))];
}

bool X264VideoCircularBuffer::pushBack() 
{
	int sizeBuffer = inputFrame->getSizeNals();
	x264_nal_t** buffer = inputFrame->getNals();
    int positionsRequested = (last + sizeBuffer) % (MAX_NALS + 1);
	int i = 0, index = 0;

    if (positionsRequested > first) {
        return false;
    }
	
	VCodecType codec = inputFrame->getCodec();
	unsigned int width = inputFrame->getWidth();
    unsigned int height = inputFrame->getHeight();
	PixType pixelFormat = inputFrame->getPixelFormat();

    for (i=0; i<sizeBuffer; i++) {
		index =  ((last+i) % (MAX_NALS + 1));
		int sizeNal = (*buffer)[i].i_payload;
		data[index] = InterleavedVideoFrame::createNew (codec, width, height, pixelFormat);
		memcpy(data[index]->getDataBuf(), (*buffer)[i].p_payload, sizeNal);
		data[index]->setLength(sizeNal);		
	}
	
	last = index;
	
    return true;
}   

bool X264VideoCircularBuffer::popFront()
{
    if (first == last)
		return false;

	outputFrame = data[first];
	first = (first + 1) % (MAX_NALS + 1); 

    return true;
}


bool X264VideoCircularBuffer::forcePushBack()
{
    return pushBack();
}

