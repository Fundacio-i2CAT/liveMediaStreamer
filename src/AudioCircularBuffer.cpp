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

#define MAX_DEVIATION_SAMPLES 1000

int mod (int a, int b);

AudioCircularBuffer* AudioCircularBuffer::createNew(int ch, int sRate, int maxSamples, SampleFmt sFmt)
{
    AudioCircularBuffer* b = new AudioCircularBuffer(ch, sRate, maxSamples, sFmt);

    if (!b->setup()) {
        utils::errorMsg("AudioCircularBuffer setup error!");
        delete b;
        return NULL;
    }

    return b;
}

AudioCircularBuffer::AudioCircularBuffer(int ch, int sRate, int maxSamples, SampleFmt sFmt)
: FrameQueue(), channels(ch), sampleRate(sRate), chMaxSamples(maxSamples), sampleFormat(sFmt),
outputFrameAlreadyRead(true), bufferingState(BUFFERING)
{
    syncTimestamp = std::chrono::microseconds(0);
    tsDeviationThreshold = MAX_DEVIATION_SAMPLES*std::micro::den/sRate;
    synchronized = false;
}

AudioCircularBuffer::~AudioCircularBuffer()
{
    for (int i=0; i<channels; i++) {
        delete[] data[i];
    }

    delete inputFrame;
    delete outputFrame;
}


Frame* AudioCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* AudioCircularBuffer::getFront(bool &newFrame)
{
    std::chrono::microseconds ts;
    unsigned frontSampleIdx;

    if (outputFrameAlreadyRead == false) {
        newFrame = true;
        return outputFrame;
    }

    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        newFrame = false;
        utils::debugMsg("There is not enough data to fill a frame. Impossible to get new frame!");
        return NULL;
    }

    frontSampleIdx = front/bytesPerSample;
    ts = std::chrono::microseconds(frontSampleIdx*std::micro::den/sampleRate) + syncTimestamp;
    outputFrame->setPresentationTime(ts);
    frontSampleIdx += outputFrame->getSamples();
    newFrame = true;
    return outputFrame;
}

void AudioCircularBuffer::addFrame()
{
    std::chrono::microseconds inTs;
    std::chrono::microseconds rearTs;
    std::chrono::microseconds deviation;
    unsigned paddingSamples;
    unsigned rearSampleIdx;

    inTs = inputFrame->getPresentationTime();

    if (!synchronized) {
        syncTimestamp = inTs;
        synchronized = true;
    }

    rearSampleIdx = rear/bytesPerSample;
    rearTs = std::chrono::microseconds(rearSampleIdx*std::micro::den/sampleRate) + syncTimestamp;
    deviation = inTs - rearTs;

    if (deviation.count() < -tsDeviationThreshold) {
        utils::warningMsg("[AudioCircularBuffer] Timestamp from the past, discarding entire frame");
        return;
    }

    if (deviation.count() > tsDeviationThreshold) {
        utils::warningMsg("[AudioCircularBuffer] Deviation exceeded, introducing silence");

        paddingSamples = (deviation.count()*sampleRate)/std::micro::den;

        if(!forcePushBack(dummyFrame->getPlanarDataBuf(), paddingSamples)) {
            //TODO::flush buffer??
            utils::warningMsg("[AudioCircularBuffer] Cannot push padding");
            return;
        }
    }

    if(!forcePushBack(inputFrame->getPlanarDataBuf(), inputFrame->getSamples())) {
        utils::warningMsg("[AudioCircularBuffer] Cannot push frame");
        return;
    }
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

Frame* AudioCircularBuffer::forceGetFront(bool &newFrame)
{
    bufferingState = OK;

    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        utils::debugMsg("There is not enough data to fill a frame. Outputing silence!");
        //return dummy frame
    }

    //updateState

    return outputFrame;
}

bool AudioCircularBuffer::setup()
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
        default:
            bytesPerSample = 0;
            utils::errorMsg("[Audio Circular Buffer] Only planar sample formats are supported (U8P, S16P, FLTP)");
            return false;
    }

    channelMaxLength = BUFFERING_SIZE_TIME*(sampleRate/1000) * bytesPerSample;

    for (int i=0; i<channels; i++) {
        data[i] = new unsigned char [channelMaxLength]();
    }

    inputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);
    outputFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);
    dummyFrame = PlanarAudioFrame::createNew(channels, sampleRate, chMaxSamples, PCM, sampleFormat);
    dummyFrame->fillWithValue(0);

    outputFrame->setSamples(AudioFrame::getDefaultSamples(sampleRate));
    outputFrame->setLength(AudioFrame::getDefaultSamples(sampleRate)*bytesPerSample);

    samplesBufferingThreshold = BUFFERING_THRESHOLD*(sampleRate/1000);

    return true;
}


bool AudioCircularBuffer::pushBack(unsigned char **buffer, int samplesRequested)
{
    unsigned bytesRequested = samplesRequested * bytesPerSample;
    unsigned rearMod;
    int firstCopiedBytes;

    if (bytesRequested > (channelMaxLength - elements)) {
        return false;
    }

    rearMod = rear % channelMaxLength;

    if ((rearMod + bytesRequested) < channelMaxLength) {

        for (int i=0; i<channels; i++) {
            memcpy(data[i] + rearMod, buffer[i], bytesRequested);
        }

    } else {

        firstCopiedBytes = channelMaxLength - rearMod;
        
        for (int i=0; i<channels;  i++) {
            memcpy(data[i] + rearMod, buffer[i], firstCopiedBytes);
            memcpy(data[i], buffer[i] + firstCopiedBytes, bytesRequested - firstCopiedBytes);
        }
    }

    elements += bytesRequested;
    rear += bytesRequested;
    return true;
}

bool AudioCircularBuffer::popFront(unsigned char **buffer, int samplesRequested)
{
    int bytesRequested = samplesRequested * bytesPerSample;

    if (elements < samplesBufferingThreshold * bytesPerSample) {
        bufferingState = BUFFERING;
    } else {
	    bufferingState = OK;
    }

    if (bufferingState == BUFFERING) {
	   return false;
    }

    fillOutputBuffers(buffer, bytesRequested);

    return true;
}

void AudioCircularBuffer::fillOutputBuffers(unsigned char **buffer, int bytesRequested)
{
    int firstCopiedBytes = 0;
    unsigned frontMod;

    frontMod = front % channelMaxLength;

    if (frontMod + bytesRequested < channelMaxLength) {
        for (int i=0; i<channels; i++) {
            memcpy(buffer[i], data[i] + frontMod, bytesRequested);
        }

    } else {
        firstCopiedBytes = channelMaxLength - frontMod;

        for (int i=0; i<channels;  i++) {
            memcpy(buffer[i], data[i] + frontMod, firstCopiedBytes);
            memcpy(buffer[i] + firstCopiedBytes, data[i], bytesRequested - firstCopiedBytes);
        }
    }

    elements -= bytesRequested;
    front += bytesRequested;
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
        utils::debugMsg("There is not enough free space in the buffer. Discarding samples");
        if (getFreeSamples() != 0) {
            pushBack(inputFrame->getPlanarDataBuf(), getFreeSamples());
        }
        return false;
    }

    return true;
}

void AudioCircularBuffer::setOutputFrameSamples(int samples) 
{
    outputFrame->setSamples(samples);
    outputFrame->setLength(samples*bytesPerSample);
}

QueueState AudioCircularBuffer::getState()
{
    float occupancy = (float)elements/(float)channelMaxLength;

    if (occupancy > FAST_THRESHOLD) {
        state = FAST;
    }

    if (occupancy < SLOW_THRESHOLD) {
        state = SLOW;
    }

    return state;
}

int mod (int a, int b)
{
    int ret = a % b;

    if (ret < 0) {
        ret += b;
    }

    return ret;
}
