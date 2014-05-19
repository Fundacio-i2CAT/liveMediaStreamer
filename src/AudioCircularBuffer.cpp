/*
 *  AudioCircularBuffer - Audio circular buffer
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

#include "AudioCircularBuffer.hh"
#include <cstring>
#include <iostream>

int mod (int a, int b)
{
    int ret = a % b;
    
    if (ret < 0) {
        ret += b;
    }
    
    return ret;
}

AudioCircularBuffer* AudioCircularBuffer::createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, SampleFmt sFmt)
{
    return new AudioCircularBuffer(ch, sRate, maxSamples, sFmt);
}


Frame* AudioCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* AudioCircularBuffer::getFront()
{
    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        std::cerr << "There is not enough data to fill a frame. Impossible to get frame!\n";
        return NULL;
    }

    outputFrameAlreadyRead = false;
    return outputFrame;
}

void AudioCircularBuffer::addFrame()
{
    forcePushBack(inputFrame->getPlanarDataBuf(), inputFrame->getSamples());
}

void AudioCircularBuffer::removeFrame()
{
    outputFrameAlreadyRead = true;
    return;
}

void AudioCircularBuffer::flush() 
{
    return;
}


Frame* AudioCircularBuffer::forceGetRear()
{
    return getRear();
}

Frame* AudioCircularBuffer::forceGetFront()
{
    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        std::cerr << "There is not enough data to fill a frame. Reusing previous frame!\n";
    }

    return outputFrame;
}

AudioCircularBuffer::AudioCircularBuffer(unsigned int ch, unsigned int sRate, unsigned int maxSamples, SampleFmt sFmt)
{
    byteCounter = 0;
    front = 0;
    rear = 0;
    channels = ch;
    sampleRate = sRate;
    this->chMaxSamples = maxSamples;
    sampleFormat = sFmt;
    outputFrameAlreadyRead = false;

    config();
}

bool AudioCircularBuffer::config()
{
    switch(sampleFormat) {
        case U8P:
            bytesPerSample = 1;
            break;
        case S16P:
            bytesPerSample = 2;
            break;
        case FLTP:
            bytesPerSample = 4;
            break;
        case S_NONE:
        case U8:
        case S16:
        case FLT:
            bytesPerSample = 0;
            std::cerr << "Only planar sample formats are supported (U8P, S16P, FLTP)\n";
            break;
    }

    channelMaxLength = chMaxSamples * bytesPerSample;

    for (int i=0; i<channels; i++) {
        data[i] = new unsigned char [channelMaxLength]();
    }

    inputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);
    outputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);

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

    if (bytesRequested > byteCounter) {
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
