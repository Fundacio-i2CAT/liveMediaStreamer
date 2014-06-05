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

VideoCircularBuffer* VideoCircularBuffer::createNew(unsigned int numNals)
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

    if (!popFront(outputFrame->getBufferNals(), outputFrame->getFrameLength())) {
		moreNals = false;
        //std::cerr << "There is not enough data to fill a frame. Impossible to get frame!\n";
        return NULL;
    }

	moreNals = true;    
    return outputFrame;
}

void VideoCircularBuffer::addFrame()
{
	outputFrameAlreadyRead = true;
    forcePushBack(inputFrame->getBufferNals(), getFrameLength());
}

void VideoCircularBuffer::removeFrame()
{
    outputFrameAlreadyRead = true;
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
    if (!popFront(outputFrame->getBufferNals(), outputFrame->getFrameLength())) {
        std::cerr << "There is not enough data to fill a frame. Reusing previous frame!\n";
    }

    return outputFrame;
}

VideoCircularBuffer::VideoCircularBuffer(unsigned int numNals)
{
    byteCounter = 0;
    maxNals = numNals;
    moreNals = false;

    config();
}

bool VideoCircularBuffer::config()
{
    inputFrame = X264VideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);
    outputFrame = InterleavedVideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);

    return true;
}

AudioCircularBuffer::~AudioCircularBuffer()
{
    for (int i=0; i<channels; i++) {
        delete[] data[i];
    }
}

bool AudioCircularBuffer::pushBack(unsigned char **buffer, int samplesRequested) 
{
    int bytesRequested = samplesRequested * bytesPerSample;

    if (bytesRequested > (channelMaxLength - byteCounter)) {
        return false;
    }

    if ((rear + bytesRequested) < channelMaxLength) {
        for (int i=0; i<channels; i++) {
            memcpy(data[i] + rear, buffer[i], bytesRequested);
        }

        byteCounter += bytesRequested;
        rear = (rear + bytesRequested) % channelMaxLength;

        return true;
    }

    int firstCopiedBytes = channelMaxLength - rear;

    for (int i=0; i<channels;  i++) {
        memcpy(data[i] + rear, buffer[i], firstCopiedBytes);
        memcpy(data[i], buffer[i] + firstCopiedBytes, bytesRequested - firstCopiedBytes);
    }

    byteCounter += bytesRequested;
    rear = (rear + bytesRequested) % channelMaxLength;

    return true;
}   

bool AudioCircularBuffer::popFront(unsigned char **buffer, int samplesRequested)
{
    int bytesRequested = samplesRequested * bytesPerSample;

    if ((bytesRequested > byteCounter) || bytesRequested == 0) {
        return false;
    }

    if (front + bytesRequested < channelMaxLength) {
        for (int i=0; i<channels; i++) {
            memcpy(buffer[i], data[i] + front, bytesRequested);
        }

        byteCounter -= bytesRequested;
        front = (front + bytesRequested) % channelMaxLength;

        return true;
    }

    int firstCopiedBytes = channelMaxLength - front;

    for (int i=0; i<channels;  i++) {
        memcpy(buffer[i], data[i] + front, firstCopiedBytes);
        memcpy(buffer[i] + firstCopiedBytes, data[i], bytesRequested - firstCopiedBytes);
    }

    byteCounter -= bytesRequested;
    front = (front + bytesRequested) % channelMaxLength;

    return true;
}

int AudioCircularBuffer::getFreeSamples()
{
    int freeBytes = channelMaxLength - byteCounter;

    if (freeBytes == 0) {
        return freeBytes;
    }

    return (freeBytes/bytesPerSample);
}

bool AudioCircularBuffer::forcePushBack(unsigned char **buffer, int samplesRequested)
{
    if(!pushBack(inputFrame->getPlanarDataBuf(), inputFrame->getSamples())) {
        std::cerr << "There is not enough free space in the buffer. Discarding " 
        << inputFrame->getSamples() - getFreeSamples() << "samples.\n";
        if (getFreeSamples() != 0) {
            pushBack(inputFrame->getPlanarDataBuf(), getFreeSamples());
        }
    }
}

void AudioCircularBuffer::setOutputFrameSamples(int samples) {
    outputFrame->setSamples(samples);
    outputFrame->setLength(samples*bytesPerSample);
} 
