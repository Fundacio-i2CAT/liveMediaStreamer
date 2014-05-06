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
 
#ifndef _AUDIOFRAME_HH
#define _AUDIOFRAME_HH

#include "Frame.hh"

class AudioFrame : public Frame {
    
    public:
        AudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, CodecType codec, SampleFmt sFmt);
        AudioFrame() {};

        void setChannelNumber(unsigned int ch);
        void setSampleRate(unsigned int sRate);
        void setSampleFormat(SampleFmt sFmt);
        void setCodec(CodecType cType);
        void setSamples(unsigned int samples);
        void setMaxSamples(unsigned int maxSamples);
        CodecType getCodec();
        SampleFmt getSampleFmt();
        unsigned int getChannels();
        unsigned int getSampleRate();
        unsigned int getSamples();
        unsigned int getMaxSamples();
        unsigned int getBytesPerSample();
              
    protected:
        unsigned int channels, sampleRate, samples, maxSamples, bytesPerSample; 
        CodecType fCodec;
        SampleFmt sampleFmt;
};

#endif