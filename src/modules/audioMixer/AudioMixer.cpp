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
#include "../../AVFramedQueue.hh"
#include "../../AudioFrame.hh"
#include <iostream>
#include <utility> 

AudioMixer::AudioMixer(int inputChannels) : ManyToOneFilter(inputChannels) {
    frameChannels = DEFAULT_CHANNELS;
    sampleRate = DEFAULT_SAMPLE_RATE;
    SampleFmt sampleFormat = S16P;

    samples.resize(AudioFrame::getMaxSamples(sampleRate));
    mixedSamples.resize(AudioFrame::getMaxSamples(sampleRate));

    for (auto id : getAvailableReaders()) {
        gains.insert(std::pair<int,float>(id, 0.6));
    }

    masterGain = 1.0;
}

AudioMixer::AudioMixer(int inputChannels, int frameChannels, int sampleRate) : ManyToOneFilter(inputChannels) {
    this->frameChannels = frameChannels;
    this->sampleRate = sampleRate;
    SampleFmt sampleFormat = S16P;

    samples.resize(AudioFrame::getMaxSamples(sampleRate));
    mixedSamples.resize(AudioFrame::getMaxSamples(sampleRate));

    for (auto id : getAvailableReaders()) {
        gains.insert(std::pair<int,float>(id, 1.0));
    }

    masterGain = 1.0;
}

FrameQueue *AudioMixer::allocQueue() {
    return AudioFrameQueue::createNew(PCM, 0, sampleRate, frameChannels, sampleFormat);
}

bool AudioMixer::doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst) {
    std::vector<int> filledFramesIds;

    for (auto frame : orgFrames) {
        if (frame.second) {
            filledFramesIds.push_back(frame.first);
        }
    }

    if (filledFramesIds.empty()) {
        return false;
    }

    mixNonEmptyFrames(orgFrames, filledFramesIds, dst, (int)orgFrames.size());

    return true;
}

void AudioMixer::mixNonEmptyFrames(std::map<int, Frame*> orgFrames, std::vector<int> filledFramesIds, Frame *dst, int totalFrames) 
{
    int nOfSamples = 0;

    for (int ch = 0; ch < frameChannels; ch++) {
        for (auto id : filledFramesIds) {
            nOfSamples = dynamic_cast<AudioFrame*>(orgFrames[id])->getChannelFloatSamples(samples, ch);

            if (mixedSamples.size() != nOfSamples) {
                mixedSamples.resize(nOfSamples);
            }

            applyGainToChannel(samples, gains[id]);
            sumValues(samples, mixedSamples);
        }

        applyMixAlgorithm(mixedSamples, totalFrames);
        applyGainToChannel(mixedSamples, masterGain);
        AudioFrame *aDst = dynamic_cast<AudioFrame*>(dst);
        aDst->setSamples(nOfSamples);
        aDst->setLength(nOfSamples*BPS);
        aDst->fillBufferWithFloatSamples(mixedSamples, ch);

        std::fill(mixedSamples.begin(), mixedSamples.end(), 0);
    }
}

void AudioMixer::applyMixAlgorithm(std::vector<float> &fSamples, int frameNumber)
{
    for (auto sample : fSamples) {
        sample *= (1.0/frameNumber);
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