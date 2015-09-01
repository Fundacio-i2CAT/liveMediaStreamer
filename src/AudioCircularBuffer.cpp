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

#define MAX_DEVIATION_SAMPLES 64

AudioCircularBuffer* AudioCircularBuffer::createNew(struct ConnectionData cData, unsigned ch, unsigned sRate, unsigned maxSamples, SampleFmt sFmt, std::chrono::milliseconds bufferingThreshold)
{
    AudioCircularBuffer* b = new AudioCircularBuffer(cData, ch, sRate, maxSamples, sFmt);

    if (!b->setup()) {
        utils::errorMsg("AudioCircularBuffer setup error!");
        delete b;
        return NULL;
    }

    b->setBufferingThreshold(bufferingThreshold);
    return b;
}

AudioCircularBuffer::AudioCircularBuffer(struct ConnectionData cData, unsigned ch, unsigned sRate, unsigned maxSamples, SampleFmt sFmt)
: FrameQueue(cData), channels(ch), sampleRate(sRate), bytesPerSample(0), chMaxSamples(maxSamples), channelMaxLength(0), 
sampleFormat(sFmt), fillNewFrame(true), samplesBufferingThreshold(0), bufferingState(BUFFERING), 
inputFrame(NULL), outputFrame(NULL), dummyFrame(NULL), synchronized(false), setupSuccess(false), tsDeviationThreshold(0), elements(0)
{

}

AudioCircularBuffer::~AudioCircularBuffer()
{
    if (setupSuccess) {

        for (unsigned i=0; i<channels; i++) {
            delete[] data[i];
        }        
        
        delete inputFrame;
        delete outputFrame;
    }
}

void AudioCircularBuffer::setBufferingThreshold(std::chrono::milliseconds th)
{
    samplesBufferingThreshold = th.count()*sampleRate/std::milli::den;
}

Frame* AudioCircularBuffer::getRear()
{
    return inputFrame;
}

Frame* AudioCircularBuffer::getFront()
{
    std::chrono::microseconds ts;
    unsigned frontSampleIdx;

    std::lock_guard<std::mutex> guard(mtx);

    if (!fillNewFrame) {
        return outputFrame;
    }

    frontSampleIdx = front/bytesPerSample;
    ts = std::chrono::microseconds(frontSampleIdx*std::micro::den/sampleRate) + syncTimestamp;
    outputFrame->setPresentationTime(ts);
    outputFrame->setOriginTime(orgTime - std::chrono::microseconds(elements/(bytesPerSample*sampleRate*1000000)));

    if (!popFront(outputFrame->getPlanarDataBuf(), outputFrame->getSamples())) {
        utils::debugMsg("There is not enough data to fill a frame. Impossible to get new frame!");
        return NULL;
    }

    fillNewFrame = false;
    return outputFrame;
}

int AudioCircularBuffer::addFrame()
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
        return -1;
    }

    if (deviation.count() > tsDeviationThreshold) {
        utils::warningMsg("[AudioCircularBuffer] Deviation exceeded, introducing silence");

        paddingSamples = (deviation.count()*sampleRate)/std::micro::den;

        if (paddingSamples >= chMaxSamples) {
            utils::warningMsg("[AudioCircularBuffer] Time discontinuity. Flushing buffer!");
            flush();
            return -1;
        }

        if(!pushBack(dummyFrame->getPlanarDataBuf(), paddingSamples)) {
            utils::warningMsg("[AudioCircularBuffer] Cannot push padding");
            return -1;
        }
    }

    if(!pushBack(inputFrame->getPlanarDataBuf(), inputFrame->getSamples())) {
        utils::warningMsg("[AudioCircularBuffer] Cannot push frame");
        return -1;
    }
    
    std::lock_guard<std::mutex> guard(mtx);
    
    orgTime = inputFrame->getOriginTime();
    
    return connectionData.rFilterId;
}

int AudioCircularBuffer::removeFrame()
{
    fillNewFrame = true;
    return connectionData.wFilterId;
}

void AudioCircularBuffer::flush()
{
    std::lock_guard<std::mutex> guard(mtx);
    rear = 0;
    front = 0;
    elements = 0;
    synchronized = false;
}


Frame* AudioCircularBuffer::forceGetRear()
{
    return getRear();
}

Frame* AudioCircularBuffer::forceGetFront()
{
    return outputFrame;
}

bool AudioCircularBuffer::setup()
{
    if (channels <= 0 || sampleRate <= 0 || chMaxSamples <= 0) {
        return false;
    }

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

    channelMaxLength = chMaxSamples*(sampleRate/1000) * bytesPerSample;

    for (unsigned i=0; i<channels; i++) {
        data[i] = new unsigned char [channelMaxLength]();
    }

    inputFrame = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), PCM, sampleFormat);
    outputFrame = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), PCM, sampleFormat);
    dummyFrame = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), PCM, sampleFormat);
    dummyFrame->fillWithValue(0);

    outputFrame->setSamples(AudioFrame::getDefaultSamples(sampleRate));
    outputFrame->setLength(AudioFrame::getDefaultSamples(sampleRate)*bytesPerSample);

    tsDeviationThreshold = MAX_DEVIATION_SAMPLES*std::micro::den/sampleRate;
    setupSuccess = true;

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

        for (unsigned i=0; i<channels; i++) {
            memcpy(data[i] + rearMod, buffer[i], bytesRequested);
        }

    } else {

        firstCopiedBytes = channelMaxLength - rearMod;
        
        for (unsigned i=0; i<channels;  i++) {
            memcpy(data[i] + rearMod, buffer[i], firstCopiedBytes);
            memcpy(data[i], buffer[i] + firstCopiedBytes, bytesRequested - firstCopiedBytes);
        }
    }

    elements += bytesRequested;
    rear += bytesRequested;
    return true;
}

bool AudioCircularBuffer::popFront(unsigned char **buffer, unsigned samplesRequested)
{
    unsigned bytesRequested = samplesRequested * bytesPerSample;

    if (elements < bytesRequested) {
        return false;
    }

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
        for (unsigned i=0; i<channels; i++) {
            memcpy(buffer[i], data[i] + frontMod, bytesRequested);
        }

    } else {
        firstCopiedBytes = channelMaxLength - frontMod;

        for (unsigned i=0; i<channels;  i++) {
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

unsigned AudioCircularBuffer::getElements() 
{
    return elements/(outputFrame->getSamples()*bytesPerSample);
};

