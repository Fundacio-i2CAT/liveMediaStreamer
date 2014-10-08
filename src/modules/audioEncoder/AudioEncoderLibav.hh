/*
 *  AudioEncoderLibav - A libav-based audio encoder
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

#ifndef _AUDIO_ENCODER_LIBAV_HH
#define _AUDIO_ENCODER_LIBAV_HH

#include <chrono>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}

#include "../../AudioFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"
#include "../../Utils.hh"

class AudioEncoderLibav : public OneToOneFilter {

public:
    AudioEncoderLibav();
    ~AudioEncoderLibav();
    
    bool doProcessFrame(Frame *org, Frame *dst);
    FrameQueue* allocQueue(int wId);

    int getSamplesPerFrame(){ return samplesPerFrame;};
    int getChannels(){ return channels;};
    int getSampleRate() {return sampleRate;};
    SampleFmt getSampleFmt() {return sampleFmt;};
    ACodecType getCodec() {return fCodec;};
    void configure(ACodecType codec, int internalChannels = DEFAULT_CHANNELS, int internalSampleRate = DEFAULT_SAMPLE_RATE);

    Reader* setReader(int readerID, FrameQueue* queue, bool sharedQueue = false);

protected:
    std::chrono::microseconds getFrameTime();

private:
    void initializeEventMap();
    int resample(AudioFrame* src, AVFrame* dst);
    bool reconfigure(AudioFrame* frame);
    bool config();
    void configEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void doGetState(Jzon::Object &filterNode);
    void setPresentationTime(Frame* dst);
    void manageFramerate();

   
    AVCodec             *codec;
    AVCodecContext      *codecCtx;
    AVFrame             *libavFrame;
    AVPacket            pkt;
    AVSampleFormat      internalLibavSampleFormat;
    SwrContext          *resampleCtx;
    AVCodecID           codecID;
    AVSampleFormat      libavSampleFmt;
    int                 gotFrame;
    bool                needsConfig;

    ACodecType          fCodec;
    SampleFmt           sampleFmt;
    SampleFmt           internalSampleFmt;

    int                 channels;
    int                 internalChannels;
    int                 sampleRate;
    int                 internalSampleRate;
    int                 samplesPerFrame;
    int                 internalBufferSize;
    unsigned char       *internalBuffer;
    unsigned char       *auxBuff[1];

    std::chrono::microseconds currentTime;
    std::chrono::microseconds frameDuration;
    std::chrono::microseconds diffTime;
    std::chrono::microseconds lastDiffTime;

    float framerateMod;
};

#endif
