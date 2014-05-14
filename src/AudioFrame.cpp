/*
 *  AudioFrame - Audio frame structure
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

#include "AudioFrame.hh"

AudioFrame::AudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    channels = ch;
    sampleRate = sRate;
    fCodec = codec;
    sampleFmt = sFmt;
    samples = 0;
    this->maxSamples = maxSamples; 

    switch(sampleFmt){
        case U8P:
        case U8:
            bytesPerSample = 1;
            break;
        case S16P:
        case S16:
            bytesPerSample = 2;
            break;
        case FLTP:
        case FLT:
            bytesPerSample = 4;
            break;
        default:
            //TODO: error
            bytesPerSample = 0;
        break;
    }
}

void AudioFrame::setChannelNumber(unsigned int ch)
{
    channels = ch;
}

void AudioFrame::setSampleRate(unsigned int sRate)
{
    sampleRate = sRate;
}

void AudioFrame::setSampleFormat(SampleFmt sFmt)
{
    sampleFmt = sFmt;
}

void AudioFrame::setCodec(ACodecType cType)
{
    fCodec = cType;
}

void AudioFrame::setSamples(unsigned int samples)
{
    this->samples = samples;
}

void AudioFrame::setMaxSamples(unsigned int maxSamples)
{
    this->maxSamples = maxSamples;
}

ACodecType AudioFrame::getCodec()
{
    return fCodec;
}

SampleFmt AudioFrame::getSampleFmt()
{
    return sampleFmt;
}

unsigned int AudioFrame::getChannels()
{
    return channels;
}

unsigned int AudioFrame::getSampleRate()
{
    return sampleRate;
}

unsigned int AudioFrame::getSamples()
{
    return samples;
}

unsigned int AudioFrame::getMaxSamples()
{
    return maxSamples;
}

unsigned int AudioFrame::getBytesPerSample()
{
    return bytesPerSample;
}

      
