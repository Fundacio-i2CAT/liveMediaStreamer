/*
 *  InterleavedAudioFrame - Interleaved audio frame structure
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

#include "InterleavedAudioFrame.hh"
#include <iostream>

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

unsigned char* InterleavedAudioFrame::getDataBuf()
{
    return frameBuff;
}

unsigned int InterleavedAudioFrame::getLength()
{
    return bufferLen;
}
    
unsigned int InterleavedAudioFrame::getMaxLength()
{
    return bufferMaxLen;
}
    
void InterleavedAudioFrame::setLength(unsigned int length)
{
    bufferLen = length;
}

bool InterleavedAudioFrame::isPlanar()
{
    return false;
}

