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

#define DEFAULT_CHANNELS 2
#define MAX_CHANNELS 2
#define DEFAULT_SAMPLE_RATE 48000
#define MAX_FRAME_TIME 100 //ms
#define DEFAULT_FRAME_TIME 20 //ms


class AudioFrame : public Frame {
    
    public:
        AudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt);
        AudioFrame() {};

        void setChannelNumber(unsigned int ch) {channels = ch;};
        void setSampleRate(unsigned int sRate) {sampleRate = sRate;};
        void setSampleFormat(SampleFmt sFmt) {sampleFmt = sFmt;};
        void setCodec(ACodecType cType) {fCodec = cType;};
        void setSamples(unsigned int samples) {this->samples = samples;};
        void setMaxSamples(unsigned int maxSamples) {this->maxSamples = maxSamples;};
        ACodecType getCodec() {return fCodec;};
        SampleFmt getSampleFmt() {return sampleFmt;};
        unsigned int getChannels() {return channels;};
        unsigned int getSampleRate() {return sampleRate;};
        unsigned int getSamples() {return samples;};
        unsigned int getMaxSamples() {return maxSamples;};
        unsigned int getBytesPerSample() {return bytesPerSample;};
        virtual int getChannelFloatSamples(std::vector<float> &samplesVec, int channel) = 0;
        virtual void fillBufferWithFloatSamples(std::vector<float> samples, int channel) = 0;
        static int getMaxSamples(int sampleRate);
        static int getDefaultSamples(int sampleRate);
        static SampleFmt getSampleFormatFromString(std::string stringSampleFmt);
              
    protected:
        unsigned int channels, sampleRate, samples, maxSamples, bytesPerSample; 
        ACodecType fCodec;
        SampleFmt sampleFmt;
};

class InterleavedAudioFrame : public AudioFrame {
    public:
        static InterleavedAudioFrame* createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt);
        unsigned char* getDataBuf() {return frameBuff;};
        unsigned int getLength() {return bufferLen;};
        unsigned int getMaxLength() {return bufferMaxLen;};
        bool isPlanar() {return false;};
        void setLength(unsigned int length) {bufferLen = length;};
        int getChannelFloatSamples(std::vector<float> &samplesVec, int channel) {}; 
        void fillBufferWithFloatSamples(std::vector<float> samples, int channel) {}; 

    private:
        InterleavedAudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt);
        unsigned char *frameBuff;
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

class PlanarAudioFrame : public AudioFrame {
    public:
        static PlanarAudioFrame* createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt);
        unsigned char** getPlanarDataBuf() {return frameBuff;};
        unsigned int getLength() {return bufferLen;};
        unsigned int getMaxLength() {return bufferMaxLen;};
        bool isPlanar() {return true;};
        void setLength(unsigned int length) {bufferLen = length;};
        int getChannelFloatSamples(std::vector<float> &samplesVec, int channel); 
        void fillBufferWithFloatSamples(std::vector<float> samples, int channel); 

    private:
        PlanarAudioFrame(unsigned int ch, unsigned int sRate, unsigned int maxSamples, ACodecType codec, SampleFmt sFmt);
        unsigned char* frameBuff[MAX_CHANNELS];
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

#endif