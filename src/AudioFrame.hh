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
#include <vector>
#include <string>

#define DEFAULT_CHANNELS 2
#define MAX_CHANNELS 2
#define DEFAULT_SAMPLE_RATE 48000
#define MAX_FRAME_TIME 100 //ms
#define DEFAULT_FRAME_TIME 20000 //us

class AudioFrame : public Frame {
    
    public:
        AudioFrame(int ch, int sRate, int maxSmpls, ACodecType codec, SampleFmt sFmt);
        AudioFrame() {};

        void setChannels(int ch) {channels = ch;};
        void setSampleRate(int sRate) {sampleRate = sRate;};
        void setSampleFormat(SampleFmt sFmt) {sampleFmt = sFmt;};
        void setCodec(ACodecType cType) {fCodec = cType;};
        void setSamples(int samples) {this->samples = samples;};
        void setMaxSamples(int maxSamples) {this->maxSamples = maxSamples;};
        ACodecType getCodec() {return fCodec;};
        SampleFmt getSampleFmt() {return sampleFmt;};
        unsigned getChannels() {return channels;};
        unsigned getSampleRate() {return sampleRate;};
        unsigned getSamples() {return samples;};
        unsigned getMaxSamples() {return maxSamples;};
        virtual void fillWithValue(int value) = 0;
        static int getMaxSamples(int sampleRate);
        static int getDefaultSamples(int sampleRate);
        std::chrono::nanoseconds getDuration() const;
              
    protected:
        unsigned channels, sampleRate, samples, maxSamples, bytesPerSample;
        ACodecType fCodec;
        SampleFmt sampleFmt;
};

class InterleavedAudioFrame : public AudioFrame {
    public:
        static InterleavedAudioFrame* createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt);
        ~InterleavedAudioFrame();
        
        unsigned char **getPlanarDataBuf() {return NULL;};
        unsigned char* getDataBuf() {return frameBuff;};
        unsigned int getLength() {return bufferLen;};
        unsigned int getMaxLength() {return bufferMaxLen;};
        bool isPlanar() {return false;};
        void setLength(unsigned int length) {bufferLen = length;};
        void fillWithValue(int value);

    protected:
        InterleavedAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt);

    private:
        unsigned char *frameBuff;
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

class PlanarAudioFrame : public AudioFrame {
    public:
        static PlanarAudioFrame* createNew(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt);
        ~PlanarAudioFrame();

        unsigned char *getDataBuf() {return NULL;};
        unsigned char** getPlanarDataBuf() {return frameBuff;};
        unsigned int getLength() {return bufferLen;};
        unsigned int getMaxLength() {return bufferMaxLen;};
        bool isPlanar() {return true;};
        void setLength(unsigned int length) {bufferLen = length;};
        void fillWithValue(int value);

    private:
        PlanarAudioFrame(int ch, int sRate, int maxSamples, ACodecType codec, SampleFmt sFmt);
        unsigned char* frameBuff[MAX_CHANNELS];
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

#endif
