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
#include <iostream>
#include <stdio.h>

AudioDecoderLibav::AudioDecoderLibav()
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
    outChannels = 0;
    inSampleRate = 0;
    outSampleRate = 0;
    inFrame = av_frame_alloc();
}

bool AudioDecoderLibav::configure(CodecType cType, SampleFmt inSFmt, int inCh, 
                                    int inSRate, SampleFmt outSFmt, int outCh, int outSRate)
{
    AVCodecID codec_id;
    AVSampleFormat inLibavSampleFmt;
    AVSampleFormat outLibavSampleFmt;
    
    fCodec = cType;
    inSampleFmt = inSFmt;
    outSampleFmt = outSFmt;
    inChannels = inCh;
    outChannels = outCh;
    inSampleRate = inSRate;
    outSampleRate = outSRate;
    
    switch(fCodec){
        case PCMU:
            codec_id = AV_CODEC_ID_PCM_MULAW;
            break;
        case OPUS_C:
            codec_id = CODEC_ID_OPUS ;
            break;
        default:
            //TODO: error
            return false;
        break;
    }
    
    av_frame_unref(inFrame);   
    
    codec = avcodec_find_decoder(codec_id);
    if (codec == NULL)
    {
        //TODO: error
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        return false;
    }

    switch(inSFmt){
        case U8:
            inLibavSampleFmt = AV_SAMPLE_FMT_U8;
            break;
        case S16:
            inLibavSampleFmt = AV_SAMPLE_FMT_S16;
            break;
        case S32:
            inLibavSampleFmt = AV_SAMPLE_FMT_S32;
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
        case S32P:
            inLibavSampleFmt = AV_SAMPLE_FMT_S32P;
            break;
        case FLTP:
            inLibavSampleFmt = AV_SAMPLE_FMT_FLTP;
            break;
        default:
            //TODO: error
            return false;
        break;
    }

    switch(outSFmt){
        case U8:
            outLibavSampleFmt = AV_SAMPLE_FMT_U8;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_U8);
            break;
        case S16:
            outLibavSampleFmt = AV_SAMPLE_FMT_S16;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            break;
        case S32:
            outLibavSampleFmt = AV_SAMPLE_FMT_S32;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
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
        case S32P:
            outLibavSampleFmt = AV_SAMPLE_FMT_S32P;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S32P);
            break;
        case FLTP:
            outLibavSampleFmt = AV_SAMPLE_FMT_FLTP;
            bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
            break;
        default:
            //TODO: error
            return false;
        break;
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

    if (swr_init(resampleCtx) < 0) {
        std::cout << "Init context failure! " << std::endl;
        return false;
    } 
    
    return true;
}

bool AudioDecoderLibav::decodeFrame(Frame* codedFrame, AudioFrame* decodedFrame)
{     
    int len, gotFrame;

    //TODO: check if decoded frame configuration is the same as decoder configuration

    pkt.size = codedFrame->getLength();
    pkt.data = codedFrame->getDataBuf();
            
    while (pkt.size > 0) {
        len = avcodec_decode_audio4(codecCtx, inFrame, &gotFrame, &pkt);

        if(len < 0) {
            //TODO: error
            return false;
        }

        if (gotFrame){
            resample(inFrame, decodedFrame);
        }
        
        if(pkt.data) {
            pkt.size -= len;
            pkt.data += len;
        }
    }
        
    return true;
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

int AudioDecoderLibav::resample(AVFrame* src, AudioFrame* dst)
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

        
        dst->setLength(outChannels*samples*bytesPerSample);
        dst->setSamples(samples);
    }
}

