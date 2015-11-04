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

void PlanarAudioFrame::fillWithValue(int value)
{
    for (unsigned i = 0; i < channels; i++) {
        memset(frameBuff[i], value, bufferMaxLen);
    }
} 