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
#include "../../AudioFrame.hh"
#include "../../Utils.hh"
#include <iostream>
#include <utility>
#include <cmath>

AudioMixer::AudioMixer(FilterRole role, int inputChannels) : ManyToOneFilter(role, inputChannels) 
{
    frameChannels = DEFAULT_CHANNELS;
    sampleRate = DEFAULT_SAMPLE_RATE;
    sampleFormat = S16P;
    maxChannels = inputChannels;
    fType = AUDIO_MIXER;

    samples.resize(AudioFrame::getMaxSamples(sampleRate));
    mixedSamples.resize(AudioFrame::getMaxSamples(sampleRate));
    samplesPerFrame = AudioFrame::getDefaultSamples(sampleRate);
    setFrameTime(std::chrono::nanoseconds(samplesPerFrame*std::nano::den/sampleRate));

    initializeEventMap();

    masterGain = DEFAULT_MASTER_GAIN;
    th = COMPRESSION_THRESHOLD;
    mAlg = LDRC;
}

AudioMixer::~AudioMixer() {}

FrameQueue *AudioMixer::allocQueue(int wFId, int rFId, int wId) 
{
    return AudioCircularBuffer::createNew(wFId, rFId, frameChannels, sampleRate, AudioFrame::getMaxSamples(sampleRate), sampleFormat);
}

bool AudioMixer::doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst)
{
    std::vector<int> filledFramesIds;

    for (auto frame : orgFrames) {
        if (frame.second) {
            filledFramesIds.push_back(frame.first);
        }
    }

    if (filledFramesIds.empty()) {
        return false;
    }

    mixNonEmptyFrames(orgFrames, filledFramesIds, dst);

    dst->setConsumed(true);
    return true;
}

void AudioMixer::mixNonEmptyFrames(std::map<int, Frame*> orgFrames, std::vector<int> filledFramesIds, Frame *dst)
{
    int nOfSamples = 0;

    for (int ch = 0; ch < frameChannels; ch++) {
        for (auto id : filledFramesIds) {
            nOfSamples = dynamic_cast<AudioFrame*>(orgFrames[id])->getChannelFloatSamples(samples, ch);

            if ((int)mixedSamples.size() != nOfSamples) {
                mixedSamples.resize(nOfSamples);
            }

            applyGainToChannel(samples, gains[id]);
            sumValues(samples, mixedSamples);
        }

        applyMixAlgorithm(mixedSamples, (int)orgFrames.size(), (int)filledFramesIds.size());
        applyGainToChannel(mixedSamples, masterGain);
        AudioFrame *aDst = dynamic_cast<AudioFrame*>(dst);
        aDst->setSamples(nOfSamples);
        aDst->setLength(nOfSamples*BPS);
        aDst->fillBufferWithFloatSamples(mixedSamples, ch);

        std::fill(mixedSamples.begin(), mixedSamples.end(), 0);
    }
    
}

void AudioMixer::applyMixAlgorithm(std::vector<float> &fSamples, int totalFrameNumber, int realFrameNumber)
{
    switch (mAlg) {
        case LA:
            LAMixAlgorithm(fSamples, totalFrameNumber);
            break;
        case LDRC:
            LDRCMixAlgorithm(fSamples, realFrameNumber);
            break;
        default:
            LAMixAlgorithm(fSamples, totalFrameNumber);
        break;
    }
}

void AudioMixer::applyGainToChannel(std::vector<float> &fSamples, float gain)
{
    for (float& sample : fSamples) {
        sample *= gain;
    }
}

void AudioMixer::sumValues(std::vector<float> fSamples, std::vector<float> &mixedSamples)
{
    int i=0;
    for (float& mixedSample : mixedSamples) {
        mixedSample += fSamples[i];
        i++;
    }
}

Reader* AudioMixer::setReader(int readerID, FrameQueue* queue)
{
    AudioCircularBuffer* circularBufferQueue;
    
    if (readers.count(readerID) > 0) {
        return NULL;
    }
    
    if (!(circularBufferQueue = dynamic_cast<AudioCircularBuffer*>(queue))){
        return NULL;
    }
    circularBufferQueue->setOutputFrameSamples(samplesPerFrame);

    Reader* r = new Reader();
    readers[readerID] = r;

    gains[readerID] = DEFAULT_CHANNEL_GAIN;

    return r;
}

void AudioMixer::LAMixAlgorithm(std::vector<float> &fSamples, int frameNumber)
{
    for (auto sample : fSamples) {
        sample *= (1.0/frameNumber);
    }
}

void AudioMixer::LDRCMixAlgorithm(std::vector<float> &fSamples, int frameNumber)
{
    for (auto s : fSamples) {
        if (abs(s) > th) {
            s = (s/abs(s))*(th + ( ((1 - th)/((float)frameNumber - th))*(abs(s) - th) ));
        }
    }
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

    if (gains.count(id) <= 0) {
        return false;
    }

    gains[id] = volume;
    return true;
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

    if (gains.count(id) <= 0) {
        return false;
    }

    gains[id] = 0;
    return true;
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

    if (gains.count(id) <= 0) {
        return false;
    }

    for (auto &it : gains) {
        if (it.first == id) {
            it.second = DEFAULT_CHANNEL_GAIN;
        } else {
            it.second = 0;
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

    filterNode.Add("sampleRate", sampleRate);
    filterNode.Add("channels", frameChannels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFormat));
    filterNode.Add("masterGain", masterGain);
    filterNode.Add("maxChannels", maxChannels);

    for (auto it : gains) {
        Jzon::Object gain;
        gain.Add("id", it.first);
        gain.Add("gain", it.second);
        jsonGains.Add(gain);
    }

    filterNode.Add("gains", jsonGains);
}
