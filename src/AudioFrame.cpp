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
#include <string.h>
#include "Utils.hh"

int AudioFrame::getMaxSamples(int sampleRate)
{
    return (MAX_FRAME_TIME*sampleRate)/1000;
}

int AudioFrame::getDefaultSamples(int sampleRate)
{
    return (DEFAULT_FRAME_TIME*sampleRate)/1000000;
}


AudioFrame::AudioFrame(int ch, int sRate, int maxSmpls, ACodecType codec, SampleFmt sFmt) : 
Frame(), channels(ch), sampleRate(sRate), samples(0), maxSamples(maxSmpls), fCodec(codec), sampleFmt(sFmt)
{
    bytesPerSample = utils::getBytesPerSampleFromFormat(sFmt);
}

std::chrono::nanoseconds AudioFrame::getDuration() const
{
    std::chrono::nanoseconds duration(samples*std::nano::den/sampleRate);
    return duration;
}


//////////////////////////////////////////////////
//INTERLEAVED AUDIO FRAME METHODS IMPLEMENTATION//
//////////////////////////////////////////////////

InterleavedAudioFrame* InterleavedAudioFrame::createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    if (sFmt != U8 && sFmt != S16 && sFmt != FLT) {
        utils::errorMsg("[InterleavedAudioFrame] Sample format not supported");
        return NULL;
    }

    return new InterleavedAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

InterleavedAudioFrame::InterleavedAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
: AudioFrame(ch, sRate, maxSamples, codec, sFmt)
{
    bufferMaxLen = bytesPerSample * maxSamples * MAX_CHANNELS;
    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedAudioFrame::~InterleavedAudioFrame() 
{
    delete[] frameBuff;
}

void InterleavedAudioFrame::fillWithValue(int value)
{
    memset(frameBuff, value, bufferMaxLen*channels);
}    



/////////////////////////////////////////////
//PLANAR AUDIO FRAME METHODS IMPLEMENTATION//
/////////////////////////////////////////////

PlanarAudioFrame* PlanarAudioFrame::createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
{
     if (sFmt != U8P && sFmt != S16P && sFmt != FLTP) {
        utils::errorMsg("[PlanarAudioFrame] Sample format not supported");
        return NULL;
    }

    return new PlanarAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

PlanarAudioFrame::PlanarAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt)
: AudioFrame(ch, sRate, maxSamples, codec, sFmt)
{
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
    short value = 0;
    float fValue = 0;

    if (sampleFmt != S16P && sampleFmt != FLTP) {
        utils::errorMsg("[PlanarAudioFrame] Only S16P and FLTP formats are supported when converting to float values");
        return 0;
    }

    if ((int)samplesVec.size() != samples) {
        samplesVec.resize(samples);
    }

    unsigned char* b = frameBuff[channel];

    if (sampleFmt == FLTP) {
        for (int i = 0; i < samples; i++) {
            fValue = *(float*)(b+i*bytesPerSample);
            samplesVec[i] = fValue;
        }

    } else if (sampleFmt == S16P) {
        for (int i=0; i < samples; i++) {
            value = (short)(b[i*bytesPerSample] | b[i*bytesPerSample + 1] << 8);
            fValue = value / 32768.0f;
            samplesVec[i] = fValue;
        }
    }

    return samples;
}

void PlanarAudioFrame::fillBufferWithFloatSamples(std::vector<float> samplesVec, int channel)
{
    short value = 0;
    unsigned char* b = frameBuff[channel];

    if (sampleFmt != S16P && sampleFmt != FLTP) {
        utils::errorMsg("[PlanarAudioFrame] Only S16P and FLTP formats are supported when converting from float values");
        return;
    }

    if (sampleFmt == FLTP) {
        for (int i = 0; i < samples; i++) {
            memcpy(b + i*bytesPerSample, &samplesVec[i], bytesPerSample);
        }

    } else if (sampleFmt == S16P) {
        for (int i = 0; i < samples; i++) {
            value = samplesVec[i] * 32768.0;
            b[i*bytesPerSample] = value & 0xFF; 
            b[i*bytesPerSample+1] = (value >> 8) & 0xFF;
        }
    }
}

void PlanarAudioFrame::fillWithValue(int value)
{
    for (int i = 0; i < channels; i++) {
        memset(frameBuff[i], value, bufferMaxLen);
    }
} 