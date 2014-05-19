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

//////////////////////////////////////////////////
//INTERLEAVED AUDIO FRAME METHODS IMPLEMENTATION//
//////////////////////////////////////////////////

InterleavedAudioFrame* InterleavedAudioFrame::createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    return new InterleavedAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

InterleavedAudioFrame::InterleavedAudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    channels = ch;
    sampleRate = sRate;
    fCodec = codec;
    sampleFmt = sFmt;
    samples = 0;
    this->maxSamples = maxSamples; 

    switch(sampleFmt){
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

    bufferMaxLen = bytesPerSample * maxSamples * channels;
    
    frameBuff = new unsigned char [bufferMaxLen]();
}

/////////////////////////////////////////////
//PLANAR AUDIO FRAME METHODS IMPLEMENTATION//
/////////////////////////////////////////////

PlanarAudioFrame* PlanarAudioFrame::createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    return new PlanarAudioFrame(ch, sRate, maxSamples, codec, sFmt);
}

PlanarAudioFrame::PlanarAudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt)
{
    channels = ch;
    sampleRate = sRate;
    fCodec = codec;
    sampleFmt = sFmt;
    samples = 0;
    this->maxSamples = maxSamples; 

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

    for (int i=0; i<channels; i++) {
        frameBuff[i] = new unsigned char [bufferMaxLen]();
    }
}

      
