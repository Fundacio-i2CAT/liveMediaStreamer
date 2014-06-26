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
    libavFrame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    fType = AUDIO_ENCODER;

    channels = DEFAULT_CHANNELS;
    sampleRate = DEFAULT_SAMPLE_RATE;
    sampleFmt = S16P;
    libavSampleFmt = AV_SAMPLE_FMT_S16P;
	gettimeofday(&presentationTime, NULL);
	timestamp = ((uint64_t)presentationTime.tv_sec * (uint64_t)1000000) + (uint64_t)presentationTime.tv_usec;

    initializeEventMap();
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

    checkInputParams(aRawFrame->getSampleFmt(), aRawFrame->getChannels(), aRawFrame->getSampleRate());
    
    //set up buffer and buffer length pointers
    pkt.data = dst->getDataBuf();
    pkt.size = dst->getMaxLength();

    //resample in order to adapt to encoder constraints
    resample(aRawFrame, libavFrame);

    ret = avcodec_encode_audio2(codecCtx, &pkt, libavFrame, &gotFrame);


    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame\n");
        return false;
    }

    if (gotFrame) {
        dst->setLength(pkt.size);
		timestamp+= (((uint64_t) (1000000 * (libavFrame->nb_samples)))/ (uint64_t) internalSampleRate);
		presentationTime.tv_sec= (timestamp / 1000000);
		presentationTime.tv_usec= (timestamp % 1000000);
		dst->setPresentationTime(presentationTime);
        return true;
    }

            
    return false;
}

Reader* AudioEncoderLibav::setReader(int readerID, FrameQueue* queue)
{
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    dynamic_cast<AudioCircularBuffer*>(queue)->setOutputFrameSamples(samplesPerFrame);

    return r;
}

bool AudioEncoderLibav::configure(ACodecType codec, int internalChannels, int internalSampleRate)
{
    fCodec = codec;
    this->internalChannels = internalChannels;
    this->internalSampleRate = internalSampleRate;

    switch(fCodec) {
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

    return config();
}

bool AudioEncoderLibav::config() 
{
    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }
    
    codec = avcodec_find_encoder(codecID);
    if (!codec) {
        fprintf(stderr, "Error finding encoder\n");
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        fprintf(stderr, "Error allocating codec context\n");
        return false;
    }

    if (fCodec != PCMU && fCodec != PCM) {
        if (!checkSampleFormat(codec, internalLibavSampleFormat)) {
            fprintf(stderr, "Encoder does not support sample format %s\n", 
                            av_get_sample_fmt_name(internalLibavSampleFormat));
            return false;
        }

        if (!checkSampleRateSupport(codec, internalSampleRate)) {
            fprintf(stderr, "Encoder does not support sample rate %d\n", internalSampleRate);
            return false;
        }

        if (!checkChannelLayoutSupport(codec, av_get_default_channel_layout(internalChannels))) {
            fprintf(stderr, "Encoder does not support channel layout %ld\n", 
                                av_get_default_channel_layout(internalChannels));
            return false;
        }
    }

    codecCtx->channels = internalChannels;
    codecCtx->channel_layout = av_get_default_channel_layout(internalChannels);
    codecCtx->sample_rate = internalSampleRate;
    codecCtx->sample_fmt = internalLibavSampleFormat;

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec context\n");
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
        fprintf(stderr, "Error allocating resample context\n");
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            fprintf(stderr, "Error initializing encoder resample context\n");
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
       fprintf(stderr, "Could not get sample buffer size\n");
       return false;
    }

    if (internalBuffer) {
        delete[] internalBuffer;
    } 

    internalBuffer = new unsigned char[internalBufferSize]();
    if (!internalBuffer) {
        fprintf(stderr, "Could not allocate %d bytes for samples buffer\n", internalBufferSize);
        return false;
    }

    /* setup the data pointers in the AVFrame */
    int ret = avcodec_fill_audio_frame(libavFrame, codecCtx->channels, codecCtx->sample_fmt,
                                        (const uint8_t*)internalBuffer, internalBufferSize, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not setup audio frame\n");
        return false;
    }

    return true;
}

bool AudioEncoderLibav::inputConfig()
{
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
        fprintf(stderr, "Error allocating resample context\n");
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            fprintf(stderr, "Error initializing encoder resample context\n");
            return false;
        } 
    }

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
}

void AudioEncoderLibav::checkInputParams(SampleFmt sampleFormat, int channels, int sampleRate)
{
    if (sampleFmt == sampleFormat && this->channels == channels && this->sampleRate == sampleRate) {
        return;
    }

    sampleFmt = sampleFormat;
    this->channels = channels;
    this->sampleRate = sampleRate;

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

    if (channels != internalChannels || sampleRate != internalSampleRate) {
        internalChannels = channels;
        internalSampleRate = sampleRate;
        config();
    } else {
        inputConfig();
    }
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

    if(!configure(fCodec, newChannels, newSampleRate)) {
        outputNode.Add("error", "Error configuring audio encoder");
    } else {
        outputNode.Add("error", Jzon::null);
    }
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




