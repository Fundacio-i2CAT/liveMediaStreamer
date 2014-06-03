/*
 *  AudioDecoderLibav - A libav-based audio decoder
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

#include "AudioDecoderLibav.hh"
#include "../../AudioCircularBuffer.hh"
#include <iostream>
#include <stdio.h>

AudioDecoderLibav::AudioDecoderLibav() : OneToOneFilter()
{
    avcodec_register_all();

    codec = NULL;
    codecCtx = NULL;
    resampleCtx = NULL;
    inFrame = NULL;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    inChannels = 0;
    inSampleRate = 0;
    inFrame = av_frame_alloc();

    configure(S16P, DEFAULT_CHANNELS, DEFAULT_SAMPLE_RATE);
}

AudioDecoderLibav::~AudioDecoderLibav()
{
    av_free(codec);
    avcodec_close(codecCtx);
    av_free(codecCtx);
    swr_free(&resampleCtx);
    av_free(inFrame);
    av_free_packet(&pkt);
}

FrameQueue* AudioDecoderLibav::allocQueue(int wId)
{
    return AudioCircularBuffer::createNew(outChannels, outSampleRate, AudioFrame::getMaxSamples(outSampleRate), outSampleFmt);
}

bool AudioDecoderLibav::doProcessFrame(Frame *org, Frame *dst)
{     
    int len, gotFrame;

    AudioFrame* aCodedFrame;
    AudioFrame* aDecodedFrame = dynamic_cast<AudioFrame*>(dst);

    aCodedFrame = dynamic_cast<AudioFrame*>(org);
    checkInputParams(aCodedFrame->getCodec(), 
                     aCodedFrame->getSampleFmt(), 
                     aCodedFrame->getChannels(), 
                     aCodedFrame->getSampleRate() 
                     );

    if (needsConfig) {
        outputConfig();
    }

    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();
            
    while (pkt.size > 0) {
        len = avcodec_decode_audio4(codecCtx, inFrame, &gotFrame, &pkt);

        if(len < 0) {
            //TODO: error
            return false;
        }

        if (gotFrame){
            if (!resample(inFrame, aDecodedFrame)) {
                std::cerr << "Resampling failed!" << std::endl;
                return false;
            }
        }
        
        if(pkt.data) {
            pkt.size -= len;
            pkt.data += len;
        }
    }
        
    return true;
}

void AudioDecoderLibav::configure(SampleFmt sampleFormat, int channels, int sampleRate)
{
    outSampleFmt = sampleFormat;
    outChannels = channels;
    outSampleRate = sampleRate;

    switch(outSampleFmt){
        case U8:
            outLibavSampleFmt = AV_SAMPLE_FMT_U8;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_U8);
            break;
        case S16:
            outLibavSampleFmt = AV_SAMPLE_FMT_S16;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            break;
        case FLT:
            outLibavSampleFmt = AV_SAMPLE_FMT_FLT;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
            break;
        case U8P:
            outLibavSampleFmt = AV_SAMPLE_FMT_U8P;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_U8P);
            break;
        case S16P:
            outLibavSampleFmt = AV_SAMPLE_FMT_S16P;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16P);
            break;
        case FLTP:
            outLibavSampleFmt = AV_SAMPLE_FMT_FLTP;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
            break;
        default:
            outLibavSampleFmt = AV_SAMPLE_FMT_NONE;
            bytesPerSample = 0;
        break;
    }

    needsConfig = true;
}

bool AudioDecoderLibav::inputConfig()
{
    av_frame_unref(inFrame);

    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }

    codec = avcodec_find_decoder(codecID);
    if (codec == NULL)
    {
        //TODO: error
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        //TODO: error
        return false;
    }

    codecCtx->channels = inChannels;
    codecCtx->channel_layout = av_get_default_channel_layout(inChannels);
    codecCtx->sample_rate = inSampleRate;
    codecCtx->sample_fmt = inLibavSampleFmt;
    
    AVDictionary* dictionary = NULL;
    if (avcodec_open2(codecCtx, codec, &dictionary) < 0)
    {
        //TODO: error
        return false;
    }

    resampleCtx = swr_alloc_set_opts
                  (
                    resampleCtx, 
                    av_get_default_channel_layout(outChannels),
                    outLibavSampleFmt, 
                    outSampleRate,
                    av_get_default_channel_layout(inChannels),
                    inLibavSampleFmt,
                    inSampleRate,
                    0,
                    NULL 
                  );  

    if (resampleCtx == NULL) {
        //TODO: error
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            std::cout << "Init context failure! " << std::endl;
            return false;
        } 
    }

    return true;
}

bool AudioDecoderLibav::outputConfig()
{
    resampleCtx = swr_alloc_set_opts
                  (
                    resampleCtx, 
                    av_get_default_channel_layout(outChannels),
                    outLibavSampleFmt, 
                    outSampleRate,
                    av_get_default_channel_layout(inChannels),
                    inLibavSampleFmt,
                    inSampleRate,
                    0,
                    NULL 
                  );  

    if (resampleCtx == NULL) {
        //TODO: error
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            std::cout << "Init context failure! " << std::endl;
            return false;
        } 
    }

    needsConfig = false;
}



bool AudioDecoderLibav::resample(AVFrame* src, AudioFrame* dst)
{
    int samples;

    if (dst->isPlanar()) {
        samples = swr_convert(
                    resampleCtx, 
                    dst->getPlanarDataBuf(), 
                    dst->getMaxSamples(), 
                    (const uint8_t**)src->data, 
                    src->nb_samples
                  );

        if (samples < 0) {
            return false;
        }

        dst->setLength(samples*bytesPerSample);
        dst->setSamples(samples);

    } else {
        
        auxBuff[0] = dst->getDataBuf();

        samples = swr_convert(
                    resampleCtx, 
                    (uint8_t**)auxBuff, 
                    dst->getMaxSamples(), 
                    (const uint8_t**)src->data, 
                    src->nb_samples
                  );

        if (samples < 0) {
            return false;
        }
        
        dst->setLength(outChannels*samples*bytesPerSample);
        dst->setSamples(samples);
    }

    return true;
}

void AudioDecoderLibav::checkInputParams(ACodecType codec, SampleFmt sampleFormat, int channels, int sampleRate)
{
    if (fCodec == codec && inSampleFmt == sampleFormat && inChannels == channels && inSampleRate == sampleRate) {
        return;
    }

    fCodec = codec;
    inSampleFmt = sampleFormat;
    inChannels = channels;
    inSampleRate = sampleRate;

     switch(fCodec) {
        case PCMU:
            codecID = AV_CODEC_ID_PCM_MULAW;
            break;
        case OPUS:
            codecID = AV_CODEC_ID_OPUS;
            break;
        default:
            codecID = AV_CODEC_ID_NONE;
            break;
    }
    
    switch(inSampleFmt) {
        case U8:
            inLibavSampleFmt = AV_SAMPLE_FMT_U8;
            break;
        case S16:
            inLibavSampleFmt = AV_SAMPLE_FMT_S16;
            break;
        case FLT:
            inLibavSampleFmt = AV_SAMPLE_FMT_FLT;
            break;
        case U8P:
            inLibavSampleFmt = AV_SAMPLE_FMT_U8P;
            break;
        case S16P:
            inLibavSampleFmt = AV_SAMPLE_FMT_S16P;
            break;
        case FLTP:
            inLibavSampleFmt = AV_SAMPLE_FMT_FLTP;
            break;
        default:
            inLibavSampleFmt = AV_SAMPLE_FMT_NONE;
            break;
    }

    inputConfig();
}

void doProcessEvent(Event event) 
{
    std::string action = e.getAction();
    Jzon::Object* params = e.getParams();

    if (action.empty()) {
        return;
    }

    if (actionMap.count(action) <= 0 {
        //TODO: error!
    }

    actionMap[action](params);
}

void configEvent(Jzon::Object* params) 
{
    SampleFmt newSampleFmt = outSampleFmt;
    int newChannels = outChannels;
    int newSampleRate = outSampleRate;

    if (!params) {
        return;
    }

    if (params->Has("sampleRate")) {
        newSampleRate = params->Get("sampleRate").ToInt();
    }

    if (params->Has("channels")) {
        newChannels = params->Get("channels").ToInt();
    }

    if (params->Has("sampleFormat")) {
        newSampleFmt = AudioFrame::getSampleFormatFromString(params->Get("sampleFormat").ToString());
    }

    configure(newSampleFmt, newChannels, newSampleRate);
}



