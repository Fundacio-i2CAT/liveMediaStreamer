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
    AudioEncoderLibav(FilterRole fRole_ = MASTER);
    ~AudioEncoderLibav();

    bool configure(ACodecType codec, int codedAudioChannels, int codedAudioSampleRate, int bitrate);
    int getSamplesPerFrame(){ return samplesPerFrame;};
    ACodecType getCodec() {return fCodec;};
    
protected:
    FrameQueue* allocQueue(struct ConnectionData cData);
    bool doProcessFrame(Frame *org, Frame *dst);
	std::shared_ptr<Reader> setReader(int readerID, FrameQueue* queue);

private:
    bool configure0(ACodecType codec, int codedAudioChannels, int codedAudioSampleRate, int bitrate);
    void initializeEventMap();
    int resample(AudioFrame* src, AVFrame* dst);
    bool reconfigure(AudioFrame* frame);
    bool resamplingConfig();
    bool codingConfig(AVCodecID codecId); 

    bool configEvent(Jzon::Node* params);
    void doGetState(Jzon::Object &filterNode);
   
    AVCodec             *codec;
    AVCodecContext      *codecCtx;
    AVFrame             *libavFrame;
    AVPacket            pkt;
    SwrContext          *resampleCtx;
    int                 gotFrame;

    ACodecType          fCodec;
    int                 samplesPerFrame;

    unsigned            internalChannels;
    unsigned            internalSampleRate;
    SampleFmt           internalSampleFmt;
    AVSampleFormat      internalLibavSampleFmt;
    int                 outputBitrate;

    unsigned            inputChannels;
    unsigned            inputSampleRate;
    SampleFmt           inputSampleFmt;
    AVSampleFormat      inputLibavSampleFmt;

    std::chrono::microseconds currentTime;
    std::chrono::microseconds frameDuration;
    std::chrono::microseconds diffTime;
    std::chrono::microseconds lastDiffTime;

    float framerateMod;
};

#endif
