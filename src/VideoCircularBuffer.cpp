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

#include "VideoCircularBuffer.hh"
#include <cstring>
#include <iostream>

VideoCircularBuffer* VideoCircularBuffer::createNew()
{
    return new VideoCircularBuffer(numNals);
}


Frame* VideoCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* VideoCircularBuffer::getFront()
{
    if (moreNals == false) {
        return NULL;
    }

    if (!popFront(outputFrame->getDataBuf(), outputFrame->getLength())) {
		moreNals = false;
        //std::cerr << "There is not enough data to fill a frame. Impossible to get frame!\n";
        return NULL;
    }

	moreNals = true;    
    return outputFrame;
}

void VideoCircularBuffer::addFrame()
{
    forcePushBack(inputFrame->getgetNals(), inputFrame->getSizeNals());
	moreNals = true;
}

void VideoCircularBuffer::removeFrame()
{
    //TODO
}

void VideoCircularBuffer::flush() 
{
    return;
}


Frame* VideoCircularBuffer::forceGetRear()
{
    return getRear();
}

Frame* VideoCircularBuffer::forceGetFront()
{
    if (!popFront(outputFrame->getDataBuf(), outputFrame->getLength())) {
        std::cerr << "There is not enough data to fill a frame. Reusing previous frame!\n";
		return NULL;
    }

    return outputFrame;
}

VideoCircularBuffer::VideoCircularBuffer()
{
	first = last = 0;    
    moreNals = false;

    config();
}

bool VideoCircularBuffer::config()
{
    inputFrame = X264VideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);
    outputFrame = InterleavedVideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);

    return true;
}

VideoCircularBuffer::~VideoCircularBuffer()
{
	int i = 0, count = (last - first);
	if (count < 0)
		count = last + ((MAX_NALS + 1) - first);
	for (i=0; i < count; i++)
		delete[] data[((first + i) % (MAX_NALS + 1))];
}

bool VideoCircularBuffer::pushBack(x264_nal_t **buffer, unsigned int sizeBuffer) 
{
    int positionsRequested = (last + sizeBuffer) % (MAX_NALS + 1);
	int i = 0, index = 0;

    if (positionsRequested > first) {
        return false;
    }

    for (i=0; i<sizeBuffer; i++) {
		index =  ((last+i) % (MAX_NALS + 1));
		int sizeNal = (*buffer)[i].i_payload;
		data[index] = new unsigned char (sizeNal);
		memcpy(data[index], (*buffer)[i].p_payload, sizeNal);
		length[index] = sizeNal;
	}
	
	last = index;
	
    return true;
}   

bool VideoCircularBuffer::popFront(unsigned char **buffer, unsigned int *length)
{
    if (first == last)
		return false;

	(*length) = length[first];
	(*buffer) = new unsigned char (length[frist]);
	memcpy ((*buffer), data[first], length[first]);
	first = (first + 1) % (MAX_NALS + 1); 

    return true;
}


bool AudioCircularBuffer::forcePushBack(x264_nal_t **buffer, unsigned int sizeBuffer)
{
    return pushBack(buffer, sizeBuffer);
}

