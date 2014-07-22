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

#include "AudioEncoderLibav.hh"
#include "../../AVFramedQueue.hh"
#include "../../AudioCircularBuffer.hh"
#include "../../Utils.hh"
#include <iostream>
#include <stdio.h>

bool checkSampleFormat(AVCodec *codec, enum AVSampleFormat sampleFmt);
bool checkSampleRateSupport(AVCodec *codec, int sampleRate);
bool checkChannelLayoutSupport(AVCodec *codec, uint64_t channelLayout);

AudioEncoderLibav::AudioEncoderLibav()  : OneToOneFilter()
{
    avcodec_register_all();

    codec = NULL;
    codecCtx = NULL;
    resampleCtx = NULL;
    libavFrame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    internalBuffer = NULL;
    pkt.size = 0;
    fType = AUDIO_ENCODER;

    internalChannels = DEFAULT_CHANNELS;
    internalSampleRate = DEFAULT_SAMPLE_RATE;
    fCodec = OPUS;
    channels = DEFAULT_CHANNELS;
    sampleRate = DEFAULT_SAMPLE_RATE;
    sampleFmt = S16P;
    libavSampleFmt = AV_SAMPLE_FMT_S16P;
    initializeEventMap();

    configure(OPUS);
    config();
}

AudioEncoderLibav::~AudioEncoderLibav()
{
    av_free(codec);
    avcodec_close(codecCtx);
    av_free(codecCtx);
    swr_free(&resampleCtx);
    av_free(libavFrame);
    av_free_packet(&pkt);
}

FrameQueue* AudioEncoderLibav::allocQueue(int wId)
{
    return AudioFrameQueue::createNew(fCodec, 0, internalSampleRate, internalChannels, internalSampleFmt);
}

bool AudioEncoderLibav::doProcessFrame(Frame *org, Frame *dst)
{     
    int ret, gotFrame;

    AudioFrame* aRawFrame = dynamic_cast<AudioFrame*>(org);

    if(!reconfigure(aRawFrame)) {
        return false;
    }
    
    //set up buffer and buffer length pointers
    pkt.data = dst->getDataBuf();
    pkt.size = dst->getMaxLength();

    //resample in order to adapt to encoder constraints
    resample(aRawFrame, libavFrame);


    ret = avcodec_encode_audio2(codecCtx, &pkt, libavFrame, &gotFrame);

    if (ret < 0) {
        utils::errorMsg("Error encoding audio frame");
        return false;
    }

    if (gotFrame) {
        setPresentationTime(dst);
        dst->setLength(pkt.size);
        return true;
    }

            
    return false;
}

Reader* AudioEncoderLibav::setReader(int readerID, FrameQueue* queue)
{
    if ((int)readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    dynamic_cast<AudioCircularBuffer*>(queue)->setOutputFrameSamples(samplesPerFrame);

    return r;
}

void AudioEncoderLibav::configure(ACodecType codec, int internalChannels, int internalSampleRate)
{
    fCodec = codec;
    this->internalChannels = internalChannels;
    this->internalSampleRate = internalSampleRate;

    switch(fCodec) {
        case PCM:
            codecID = AV_CODEC_ID_PCM_S16BE; 
            internalLibavSampleFormat = AV_SAMPLE_FMT_S16;
            internalSampleFmt = S16;
            break;
        case PCMU:
            codecID = AV_CODEC_ID_PCM_MULAW;
            internalLibavSampleFormat = AV_SAMPLE_FMT_S16;
            internalSampleFmt = S16;
            break;
        case OPUS:
            codecID = AV_CODEC_ID_OPUS;
            internalLibavSampleFormat = AV_SAMPLE_FMT_S16;
            internalSampleFmt = S16;
            break; 
        case AAC:
            codecID = AV_CODEC_ID_AAC;
            internalLibavSampleFormat = AV_SAMPLE_FMT_S16;
            internalSampleFmt = S16;
            break;
        case MP3:
            codecID = AV_CODEC_ID_MP3;
            internalLibavSampleFormat = AV_SAMPLE_FMT_S16P;
            internalSampleFmt = S16P;
            break;
        default:
            codecID = AV_CODEC_ID_NONE;
            break;
    }

    needsConfig = true;
}

bool AudioEncoderLibav::config() 
{
    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }
    
    codec = avcodec_find_encoder(codecID);
    if (!codec) {
        utils::errorMsg("Error finding encoder");
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        utils::errorMsg("Error allocating codec context");
        return false;
    }

    if (fCodec != PCMU && fCodec != PCM) {
        if (!checkSampleFormat(codec, internalLibavSampleFormat)) {
            utils::errorMsg("Encoder does not support sample format");
            return false;
        }

        if (!checkSampleRateSupport(codec, internalSampleRate)) {
            utils::errorMsg("Encoder does not support sample rate " + std::to_string(internalSampleRate));
            return false;
        }

        if (!checkChannelLayoutSupport(codec, av_get_default_channel_layout(internalChannels))) {
            utils::errorMsg("Encoder does not support channel layout");
            return false;
        }
    }

    codecCtx->channels = internalChannels;
    codecCtx->channel_layout = av_get_default_channel_layout(internalChannels);
    codecCtx->sample_rate = internalSampleRate;
    codecCtx->sample_fmt = internalLibavSampleFormat;

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        utils::errorMsg("Could not open codec context");
        return false;
    }

    resampleCtx = swr_alloc_set_opts
                  (
                    resampleCtx, 
                    av_get_default_channel_layout(internalChannels),
                    internalLibavSampleFormat, 
                    internalSampleRate,
                    av_get_default_channel_layout(channels),
                    libavSampleFmt,
                    sampleRate,
                    0,
                    NULL 
                  );

    if (resampleCtx == NULL) {
        utils::errorMsg("Error allocating resample context");
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            utils::errorMsg("Error initializing encoder resample context");
            return false;
        } 
    }

    if (codecCtx->frame_size != 0) {
        libavFrame->nb_samples = codecCtx->frame_size;
    } else {
        libavFrame->nb_samples = AudioFrame::getDefaultSamples(sampleRate);
    }

    libavFrame->format = codecCtx->sample_fmt;
    libavFrame->channel_layout = codecCtx->channel_layout;

    samplesPerFrame = libavFrame->nb_samples;

    /* the codec gives us the frame size, in samples,
    * we calculate the size of the samples buffer in bytes */
    internalBufferSize = av_samples_get_buffer_size(NULL, codecCtx->channels, 
                                                      libavFrame->nb_samples, codecCtx->sample_fmt, 0);

    if (internalBufferSize < 0) {
        utils::errorMsg("Could not get sample buffer size");
       return false;
    }

    if (internalBuffer) {
        delete[] internalBuffer;
    } 

    internalBuffer = new unsigned char[internalBufferSize]();
    if (!internalBuffer) {
        utils::errorMsg("Could not allocate " + 
            std::to_string(internalBufferSize) + 
            " bytes for samples buffer");
        return false;
    }

    /* setup the data pointers in the AVFrame */
    int ret = avcodec_fill_audio_frame(libavFrame, codecCtx->channels, codecCtx->sample_fmt,
                                        (const uint8_t*)internalBuffer, internalBufferSize, 0);
    if (ret < 0) {
        utils::errorMsg("Could not setup audio frame");
        return false;
    }

    needsConfig = false;
    
    std::chrono::high_resolution_clock::time_point tp = std::chrono::high_resolution_clock::now();
    currentTime = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch());

    return true;
}

int AudioEncoderLibav::resample(AudioFrame* src, AVFrame* dst)
{
    int samples;

    if (src->isPlanar()) {
        samples = swr_convert(
                    resampleCtx, 
                    dst->data, 
                    dst->nb_samples,
                    (const uint8_t**)src->getPlanarDataBuf(), 
                    src->getSamples() 
                  );

    } else {
        auxBuff[0] = src->getDataBuf();

        samples = swr_convert(
                    resampleCtx, 
                    dst->data, 
                    dst->nb_samples,
                    (const uint8_t**)auxBuff, 
                    src->getSamples() 
                  );

    }

    return samples;
}

bool AudioEncoderLibav::reconfigure(AudioFrame* frame)
{    
    if (sampleFmt != frame->getSampleFmt() || 
        channels != frame->getChannels() || 
        sampleRate != frame->getSampleRate() ||
        needsConfig)
    {
        sampleFmt = frame->getSampleFmt();
        channels = frame->getChannels();
        sampleRate = frame->getSampleRate();

        switch(sampleFmt){
            case U8:
                libavSampleFmt = AV_SAMPLE_FMT_U8;
                break;
            case S16:
                libavSampleFmt = AV_SAMPLE_FMT_S16;
                break;
            case FLT:
                libavSampleFmt = AV_SAMPLE_FMT_FLT;
                break;
            case U8P:
                libavSampleFmt = AV_SAMPLE_FMT_U8P;
                break;
            case S16P:
                libavSampleFmt = AV_SAMPLE_FMT_S16P;
                break;
            case FLTP:
                libavSampleFmt = AV_SAMPLE_FMT_FLTP;
                break;
            default:
                libavSampleFmt = AV_SAMPLE_FMT_NONE;
                break;
        }

        return config();

    } 

    return true;
}

void AudioEncoderLibav::setPresentationTime(Frame* dst) 
{
    std::chrono::microseconds frameDuration(1000000*libavFrame->nb_samples/internalSampleRate);
    currentTime += frameDuration;
    presentationTime.tv_sec= currentTime.count()/1000000;
    presentationTime.tv_usec= currentTime.count()%1000000;
    dst->setPresentationTime(presentationTime);
}

void AudioEncoderLibav::configEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    int newChannels = internalChannels;
    int newSampleRate = internalSampleRate;

    if (!params) {
        outputNode.Add("error", "Error configuring audio encoder");
        return;
    }

    if (params->Has("sampleRate")) {
        newSampleRate = params->Get("sampleRate").ToInt();
    }

    if (params->Has("channels")) {
        newChannels = params->Get("channels").ToInt();
    }

    configure(fCodec, newChannels, newSampleRate);

    outputNode.Add("error", Jzon::null);
}

void AudioEncoderLibav::initializeEventMap()
{
    eventMap["configure"] = std::bind(&AudioEncoderLibav::configEvent, this, std::placeholders::_1, std::placeholders::_2);
}

void AudioEncoderLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", internalSampleRate);
    filterNode.Add("channels", internalChannels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFmt));
}

bool checkSampleFormat(AVCodec *codec, enum AVSampleFormat sampleFmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sampleFmt) {
            return true;
        }
        p++;
    }

    return false;
}

bool checkSampleRateSupport(AVCodec *codec, int sampleRate)
{
    const int *p;

    if (!codec->supported_samplerates) {
        return false;
    }
    
    p = codec->supported_samplerates;
    while (*p) {
        if (*p == sampleRate) {
            return true;
        }
        p++;
    }

    return false;
}

bool checkChannelLayoutSupport(AVCodec *codec, uint64_t channelLayout)
{
    const uint64_t *p;

    if (!codec->channel_layouts)
        return false;

    p = codec->channel_layouts;
    while (*p) {
        if (*p == channelLayout) {
            return true;
        }
        p++;
    }

    return false;
}




