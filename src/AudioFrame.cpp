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
#include <iostream>
#include <assert.h>

int AudioFrame::getMaxSamples(int sampleRate)
{
    return (MAX_FRAME_TIME*sampleRate)/1000;
}

int AudioFrame::getDefaultSamples(int sampleRate)
{
    return (DEFAULT_FRAME_TIME*sampleRate)/1000000;
}


AudioFrame::AudioFrame(int ch, int sRate, int maxSamples, ACodecType codec)
{
    channels = ch;
    sampleRate = sRate;
    fCodec = codec;
    samples = 0;
    this->maxSamples = maxSamples; 
}

std::chrono::microseconds AudioFrame::getDuration() const
{
    std::chrono::microseconds duration(samples*1000000/sampleRate);
    return duration;
}


//////////////////////////////////////////////////
//INTERLEAVED AUDIO FRAME METHODS IMPLEMENTATION//
//////////////////////////////////////////////////

InterleavedAudioFrame* InterleavedAudioFrame::createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    return new InterleavedAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

InterleavedAudioFrame::InterleavedAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
: AudioFrame(ch, sRate, maxSamples, codec)
{
    sampleFmt = sFmt;

    switch(sampleFmt) {
        case U8:
            bytesPerSample = 1;
            break;
        case S16:
            bytesPerSample = 2;
            break;
        case FLT:
            bytesPerSample = 4;
            break;
        default:
            //TODO: error
            bytesPerSample = 0;
        break;
    }

    bufferMaxLen = bytesPerSample * maxSamples * MAX_CHANNELS;

    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedAudioFrame::~InterleavedAudioFrame() 
{
    delete[] frameBuff;
}

void InterleavedAudioFrame::setDummy()
{
    
}



/////////////////////////////////////////////
//PLANAR AUDIO FRAME METHODS IMPLEMENTATION//
/////////////////////////////////////////////

PlanarAudioFrame* PlanarAudioFrame::createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    return new PlanarAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

PlanarAudioFrame::PlanarAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
: AudioFrame(ch, sRate, maxSamples, codec)
{
    sampleFmt = sFmt;

    switch(sampleFmt){
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
            //TODO: error
            bytesPerSample = 0;
        break;
    }

    bufferMaxLen = bytesPerSample * maxSamples;

    for (int i=0; i<MAX_CHANNELS; i++) {
        frameBuff[i] = new unsigned char [bufferMaxLen]();
    }
}

PlanarAudioFrame::~PlanarAudioFrame()
{
    for (int i = 0; i < MAX_CHANNELS; i++) {
        delete[] frameBuff[i];
    }
}


int PlanarAudioFrame::getChannelFloatSamples(std::vector<float> &samplesVec, int channel) 
{
    assert (sampleFmt == S16P);

    if ((int)samplesVec.size() != samples) {
        samplesVec.resize(samples);
    }

    short value = 0;
    float fValue = 0;
    int samplesIndex = 0;

    unsigned char* b = frameBuff[channel];

    for (int i=0; i < samples*bytesPerSample; i+=bytesPerSample) {
        value = (short)(b[i] | b[i+1] << 8);
        fValue = value / 32768.0f;
        samplesVec[samplesIndex] = fValue;
        samplesIndex++;
    }

    return samples;
}

void PlanarAudioFrame::fillBufferWithFloatSamples(std::vector<float> samplesVec, int channel)
{
    assert (sampleFmt == S16P);

    short value = 0;
    int samplesIndex = 0;

    unsigned char* b = frameBuff[channel];

    for (int i=0; i<samples*bytesPerSample; i+=bytesPerSample) {
        value = samplesVec[samplesIndex] * 32768.0;
        b[i] = value & 0xFF; 
        b[i+1] = (value >> 8) & 0xFF;
        samplesIndex++;
    }
}

void PlanarAudioFrame::setDummy()
{
    
} 