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

void X264VideoCircularBuffer::addFrame()
{
    forcePushBack();
}

Frame* X264VideoCircularBuffer::forceGetRear()
{
    return inputFrame;
}


X264VideoCircularBuffer::X264VideoCircularBuffer(): VideoFrameQueue(H264, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422)
{
    config();
}

bool X264VideoCircularBuffer::config()
{
    inputFrame = X264VideoFrame::createNew(codec, width, height, pixelFormat);
    return true;
}

bool X264VideoCircularBuffer::pushBack() 
{
	int sizeBuffer = inputFrame->getSizeNals();
	x264_nal_t** buffer = inputFrame->getNals();

	int i = 0;


	VCodecType codec = inputFrame->getCodec();
	unsigned int width = inputFrame->getWidth();
    unsigned int height = inputFrame->getHeight();
	PixType pixelFormat = inputFrame->getPixelFormat();

    for (i=0; i<sizeBuffer; i++) {
		int sizeNal = (*buffer)[i].i_payload;
		Frame* interleavedVideoFrame= VideoFrameQueue::getRear();
		memcpy(interleavedVideoFrame->getDataBuf(), (*buffer)[i].p_payload, sizeNal);
		VideoFrameQueue::addFrame();		
	}
	
    return true;
}   

bool X264VideoCircularBuffer::forcePushBack()
{
    return pushBack();
}

