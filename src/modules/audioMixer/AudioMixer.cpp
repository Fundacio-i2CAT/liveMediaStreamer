/*
 *  AudioMixer - Audio mixer structure
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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

#define BPS 2

#include "AudioMixer.hh"
#include "../../AudioCircularBuffer.hh"
#include "../../Utils.hh"
#include <iostream>
#include <utility>
#include <cmath>
#include <string.h>

AudioMixer::AudioMixer(FilterRole role, bool sharedFrames, int inputChannels) : 
ManyToOneFilter(role, sharedFrames, inputChannels), channels(DEFAULT_CHANNELS),
sampleRate(DEFAULT_SAMPLE_RATE), sampleFormat(FLTP), maxMixingChannels(inputChannels),
front(0), rear(0), masterGain(DEFAULT_MASTER_GAIN), th(COMPRESSION_THRESHOLD), mAlg(LDRC), 
syncTs(std::chrono::microseconds(-1))
{
    fType = AUDIO_MIXER;
    inputFrameSamples = AudioFrame::getDefaultSamples(sampleRate);
    outputSamples = inputFrameSamples;
    mixBufferMaxSamples = inputFrameSamples*5;
    mixingThreshold = inputFrameSamples*3;

    for (int i = 0; i < MAX_CHANNELS; i++) {
        mixBuffers[i] = new float[mixBufferMaxSamples]();
    }

    initializeEventMap();
}

AudioMixer::~AudioMixer() 
{

}

FrameQueue *AudioMixer::allocQueue(int wId) 
{
    return AudioCircularBuffer::createNew(channels, sampleRate, DEFAULT_BUFFER_SIZE, 
                                            sampleFormat, std::chrono::milliseconds(BUFFERING_THRESHOLD));
}

bool AudioMixer::doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst) 
{
    AudioFrame* aFrame;
    AudioFrame* aDstFrame;

    for (auto frame : orgFrames) {

        if (!frame.second) {
            continue;
        }

        aFrame = dynamic_cast<AudioFrame*>(frame.second);

        if (!aFrame) {
            utils::errorMsg("[AudioMixer] Input frames must be AudioFrames");
            continue;
        }


        if (!pushToBuffer(frame.first, aFrame)) {
            utils::errorMsg("[AudioMixer] Error pushing samples to the internal buffer");
            continue;
        }
    }

    aDstFrame = dynamic_cast<AudioFrame*>(dst);

    if (!aDstFrame) {
        utils::errorMsg("[AudioMixer] Output frame must be an AudioFrame");
        return false;
    }

    if (!extractMixedFrame(aDstFrame)) {
        return false;
    }

    return true;
}

bool AudioMixer::pushToBuffer(int id, AudioFrame* frame) 
{
    unsigned char* b;
    float fSample;
    SampleFmt fmt;
    int nOfSamples;
    int bytesPerSample;
    unsigned absolutePosition;
    unsigned bufferIdx;

    bytesPerSample = utils::getBytesPerSampleFromFormat(sampleFormat);
    fmt = frame->getSampleFmt();
    nOfSamples = frame->getSamples();

    if (syncTs.count() < 0) {
        syncTs = frame->getPresentationTime();
    }

    absolutePosition = (frame->getPresentationTime() - syncTs).count()*sampleRate/std::micro::den;

    if (absolutePosition < front) {
        utils::errorMsg("[AudioMixer] Samples from the past ignored");
        return false;
    }

    for (int i = 0; i < channels; i++) {

        b = frame->getPlanarDataBuf()[i];
        bufferIdx = absolutePosition % mixBufferMaxSamples;

        for (int j = 0; j < nOfSamples; j++) {

            if (!bytesToFloat(b + j*bytesPerSample, fSample, fmt)) {
                utils::errorMsg("[AudioMixer] Error converting samples from bytes to float");
                return false;
            }

            mixSample(fSample, bufferIdx);
            bufferIdx = (bufferIdx + 1) % mixBufferMaxSamples;
        }
    }

    if (absolutePosition + nOfSamples > rear) {
        rear = absolutePosition + nOfSamples;
    }

    return true;
}

void AudioMixer::mixSample(float sample, int bufferPosition)
{
    //TODO: mix doing a lineal compression
    // if (x < th) y = x;
    // else y = lineal compression
}

bool AudioMixer::extractMixedFrame(AudioFrame* frame)
{
    unsigned mixedElements = rear - front;
    unsigned pos;
    unsigned char* b;
    float *mixB;
    std::chrono::microseconds ts;
    unsigned bytesPerSample;

    if (mixedElements < mixingThreshold) {
        return false;
    }

    bytesPerSample = utils::getBytesPerSampleFromFormat(sampleFormat);

    if (bytesPerSample <= 0) {
        utils::errorMsg("[AudioMixer] Only S16P and FLTP sample formats are supported");
        return false;
    }

    for (int i = 0; i < channels; i++) {

        b = frame->getPlanarDataBuf()[i];
        pos = front % mixBufferMaxSamples;
        mixB = mixBuffers[i];

        for (unsigned j = 0; j < outputSamples; j++) {

            if (!floatToBytes(b + j*bytesPerSample, mixB[(pos+j)%mixBufferMaxSamples], sampleFormat)) {
                utils::errorMsg("[AudioMixer] Error converting samples from bytes to float");
                return false;
            }
        }
    }

    front += outputSamples;

    ts = std::chrono::microseconds(front * std::micro::den/sampleRate) + syncTs;
    frame->setPresentationTime(ts);
    frame->setLength(outputSamples*bytesPerSample);
    frame->setSamples(outputSamples);
    frame->setChannelNumber(channels);
    frame->setSampleRate(sampleRate);

    return true;
}

bool AudioMixer::bytesToFloat(unsigned char const* origin, float &dst, SampleFmt fmt) 
{
    short value;

    switch(fmt) {
        case S16P:
            value = (short)(*origin | *(origin+1) << 8);
            dst = value / 32768.0f;
            break;
        case FLTP:
            dst = *(float*)(origin);
             break;
        default:
            return false;
    }

    return true;
}

bool AudioMixer::floatToBytes(unsigned char* dst, float const origin, SampleFmt fmt) 
{
    short value;

    switch(fmt) {
        case S16P:
            value = origin * 32768.0;
            *dst = value & 0xFF; 
            *(dst+1) = (value >> 8) & 0xFF;
            break;
        case FLTP:
            memcpy(dst, &origin, utils::getBytesPerSampleFromFormat(fmt));
            break;
        default:
            return false;
    }

    return true;
}

Reader* AudioMixer::setReader(int readerID, FrameQueue* queue)
{
    AudioCircularBuffer* inBuffer;

    if (readers.count(readerID) > 0) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    inBuffer = dynamic_cast<AudioCircularBuffer*>(queue);

    if (!inBuffer) {
        utils::errorMsg("[AudioMixer] Error setting reader: queue must be an AudioCircularBuffer");
        return NULL;
    }

    inBuffer->setOutputFrameSamples(inputFrameSamples);

    gains[readerID] = DEFAULT_CHANNEL_GAIN;

    return r;
}

bool AudioMixer::setChannelGain(int id, float value)
{
    if (gains.count(id) <= 0) {
        return false;
    }

    if (value > 1) {
        gains[id] = 1;
    } else if (value < 0) {
        gains[id] = 0;
    } else {
        gains[id] = value;
    }

    return true;
}


bool AudioMixer::changeChannelVolumeEvent(Jzon::Node* params)
{
    int id;
    float volume;

    if (!params) {
        return false;
    }

    if (!params->Has("id") || !params->Has("volume")) {
        return false;
    }

    id = params->Get("id").ToInt();
    volume = params->Get("volume").ToFloat();

    return setChannelGain(id, volume);
}

bool AudioMixer::muteChannelEvent(Jzon::Node* params)
{
    int id;

    if (!params) {
        return false;
    }

    if (!params->Has("id")) {
        return false;
    }

    id = params->Get("id").ToInt();

    return setChannelGain(id, 0);
}

bool AudioMixer::soloChannelEvent(Jzon::Node* params)
{
    int id;

    if (!params) {
        return false;
    }

    if (!params->Has("id")) {
        return false;
    }

    id = params->Get("id").ToInt();

    if (!setChannelGain(id, DEFAULT_CHANNEL_GAIN)) {
        return false;
    }

    for (auto it : gains) {
        if (it.first != id) {
            setChannelGain(it.first, 0);
        }
    }

    return true;
}

bool AudioMixer::changeMasterVolumeEvent(Jzon::Node* params)
{
    float volume;

    if (!params) {
        return false;
    }

    if (!params->Has("volume")) {
        return false;
    }

    volume = params->Get("volume").ToFloat();
    masterGain = volume;

    return true;
}

bool AudioMixer::muteMasterEvent(Jzon::Node* params)
{
    masterGain = 0;
    return true;
}

void AudioMixer::initializeEventMap()
{
    eventMap["changeChannelVolume"] = std::bind(&AudioMixer::changeChannelVolumeEvent,
                                                 this, std::placeholders::_1);

    eventMap["muteChannel"] = std::bind(&AudioMixer::muteChannelEvent, this,
                                         std::placeholders::_1);

    eventMap["soloChannel"] = std::bind(&AudioMixer::soloChannelEvent, this,
                                         std::placeholders::_1);

    eventMap["changeMasterVolume"] = std::bind(&AudioMixer::changeMasterVolumeEvent, this,
                                                std::placeholders::_1);

    eventMap["muteMaster"] = std::bind(&AudioMixer::muteMasterEvent, this,
                                        std::placeholders::_1);
}

void AudioMixer::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array jsonGains;

    filterNode.Add("channels", channels);
    filterNode.Add("sampleRate", sampleRate);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFormat));
    filterNode.Add("maxChannels", maxMixingChannels);
    filterNode.Add("masterGain", masterGain);

    for (auto it : gains) {
        Jzon::Object gain;
        gain.Add("id", it.first);
        gain.Add("gain", it.second);
        jsonGains.Add(gain);
    }

    filterNode.Add("gains", jsonGains);
}