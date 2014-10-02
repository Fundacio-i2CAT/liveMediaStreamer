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
#include "Utils.hh"
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

AudioCircularBuffer* AudioCircularBuffer::createNew(int ch, int sRate, int maxSamples, SampleFmt sFmt)
{
    return new AudioCircularBuffer(ch, sRate, maxSamples, sFmt);
}


Frame* AudioCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* AudioCircularBuffer::getFront()
{
    if (outputFrameAlreadyRead == false) {
        return outputFrame;
    }

    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        utils::debugMsg("There is not enough data to fill a frame. Impossible to get new frame!");
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
    state = OK;
    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        utils::debugMsg("There is not enough data to fill a frame. Reusing previous frame!");
        return NULL;
    }

    return outputFrame;
}

AudioCircularBuffer::AudioCircularBuffer(int ch, int sRate, int maxSamples, SampleFmt sFmt)
{
    elements = 0;
    front = 0;
    rear = 0;
    channels = ch;
    sampleRate = sRate;
    this->chMaxSamples = maxSamples;
    sampleFormat = sFmt;
    outputFrameAlreadyRead = true;
    state = BUFFERING;

    config();
}

AudioCircularBuffer::~AudioCircularBuffer()
{
    for (int i=0; i<channels; i++) {
        delete[] data[i];
    }

    delete inputFrame;
    delete outputFrame;
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
            utils::errorMsg("[Audio Circular Buffer] Only planar sample formats are supported (U8P, S16P, FLTP)");
            break;
    }

    channelMaxLength = BUFFERING_SIZE_TIME*(sampleRate/1000) * bytesPerSample;

    for (int i=0; i<channels; i++) {
        data[i] = new unsigned char [channelMaxLength]();
    }

    inputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);
    outputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);

    outputFrame->setSamples(AudioFrame::getDefaultSamples(sampleRate));
    outputFrame->setLength(AudioFrame::getDefaultSamples(sampleRate)*bytesPerSample);

    samplesBufferingThreshold = BUFFERING_THRESHOLD*(sampleRate/1000);

    return true;
}


bool AudioCircularBuffer::pushBack(unsigned char **buffer, int samplesRequested) 
{
    int bytesRequested = samplesRequested * bytesPerSample;

    if (bytesRequested > (channelMaxLength - elements)) {
        return false;
    }

    if ((rear + bytesRequested) < channelMaxLength) {
        for (int i=0; i<channels; i++) {
            memcpy(data[i] + rear, buffer[i], bytesRequested);
        }

        elements += bytesRequested;
        rear = (rear + bytesRequested) % channelMaxLength;

        return true;
    }

    int firstCopiedBytes = channelMaxLength - rear;

    for (int i=0; i<channels;  i++) {
        memcpy(data[i] + rear, buffer[i], firstCopiedBytes);
        memcpy(data[i], buffer[i] + firstCopiedBytes, bytesRequested - firstCopiedBytes);
    }

    elements += bytesRequested;
    rear = (rear + bytesRequested) % channelMaxLength;

    return true;
}   

bool AudioCircularBuffer::popFront(unsigned char **buffer, int samplesRequested)
{
    int bytesRequested = samplesRequested * bytesPerSample;
    
    if (((bytesRequested > elements) || bytesRequested == 0) && state == OK) {
        state = BUFFERING;
    }

    if (state == BUFFERING) {
        if (elements > samplesBufferingThreshold * bytesPerSample) {
            state = OK;
        }

        return false;
    }
    
    fillOutputBuffers(buffer, bytesRequested);

    return true;
}

void AudioCircularBuffer::fillOutputBuffers(unsigned char **buffer, int bytesRequested)
{
    int firstCopiedBytes = 0;

    if (front + bytesRequested < channelMaxLength) {
        for (int i=0; i<channels; i++) {
            memcpy(buffer[i], data[i] + front, bytesRequested);
        }

    } else {
        firstCopiedBytes = channelMaxLength - front;

        for (int i=0; i<channels;  i++) {
            memcpy(buffer[i], data[i] + front, firstCopiedBytes);
            memcpy(buffer[i] + firstCopiedBytes, data[i], bytesRequested - firstCopiedBytes);
        }
    }

    elements -= bytesRequested;
    front = (front + bytesRequested) % channelMaxLength;
}

int AudioCircularBuffer::getFreeSamples()
{
    int freeBytes = channelMaxLength - elements;

    if (freeBytes == 0) {
        return freeBytes;
    }

    return (freeBytes/bytesPerSample);
}

bool AudioCircularBuffer::forcePushBack(unsigned char **buffer, int samplesRequested)
{
    if(!pushBack(inputFrame->getPlanarDataBuf(), inputFrame->getSamples())) {
        utils::errorMsg("There is not enough free space in the buffer. Discarding samples");
        std::cout << "Discarding queue: " << this << std::endl;
        if (getFreeSamples() != 0) {
            pushBack(inputFrame->getPlanarDataBuf(), getFreeSamples());
        }
        return false;
    }
    return true;
}

void AudioCircularBuffer::setOutputFrameSamples(int samples) {
    outputFrame->setSamples(samples);
    outputFrame->setLength(samples*bytesPerSample);
} 
