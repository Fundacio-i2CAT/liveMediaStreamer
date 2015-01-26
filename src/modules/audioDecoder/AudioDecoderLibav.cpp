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
#include "../../Utils.hh"
#include <functional>
#include <fstream>

std::chrono::system_clock::time_point previousPoint;
std::chrono::system_clock::time_point now;
std::chrono::microseconds enlapsedTime;


AVSampleFormat getAVSampleFormatFromtIntCode(int id) 
{
    AVSampleFormat sampleFmt = AV_SAMPLE_FMT_NONE;

    if (id == AV_SAMPLE_FMT_U8) {
        sampleFmt = AV_SAMPLE_FMT_U8;
    } else if (id == AV_SAMPLE_FMT_S16) {
        sampleFmt = AV_SAMPLE_FMT_S16;
    } else if (id == AV_SAMPLE_FMT_FLT) {
        sampleFmt = AV_SAMPLE_FMT_FLT;
    } else if (id == AV_SAMPLE_FMT_U8P) {
        sampleFmt = AV_SAMPLE_FMT_U8P;
    } else if (id == AV_SAMPLE_FMT_S16P) {
        sampleFmt = AV_SAMPLE_FMT_S16P;
    } else if (id == AV_SAMPLE_FMT_FLTP) {
        sampleFmt = AV_SAMPLE_FMT_FLTP;
    }

    return sampleFmt;
}

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
    fType = AUDIO_DECODER;

    inChannels = 0;
    inSampleRate = 0;
    inFrame = av_frame_alloc();

    initializeEventMap();

    configure(S16P, DEFAULT_CHANNELS, DEFAULT_SAMPLE_RATE);
}

AudioDecoderLibav::~AudioDecoderLibav()
{
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

    AudioFrame* aCodedFrame = dynamic_cast<AudioFrame*>(org);
    AudioFrame* aDecodedFrame = dynamic_cast<AudioFrame*>(dst);

    reconfigureDecoder(aCodedFrame);

    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();


    while (pkt.size > 0) {
        len = avcodec_decode_audio4(codecCtx, inFrame, &gotFrame, &pkt);

        if(len < 0) {
            //TODO: error
            return false;
        }

        if (gotFrame) {
            now = std::chrono::system_clock::now();
            enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(now - previousPoint);
            previousPoint = now;

            checkInputParams(inFrame->format, inFrame->channels, inFrame->sample_rate);

            if (resample(inFrame, aDecodedFrame)) {
                return true;
            }
        }
        
        if(pkt.data) {
            pkt.size -= len;
            pkt.data += len;
        }
    }
        
    return false;
}

bool AudioDecoderLibav::configure(SampleFmt sampleFormat, int channels, int sampleRate)
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

    return outputConfig();

}

bool AudioDecoderLibav::inputConfig()
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
        utils::errorMsg("[DECODER] Error allocating resample context!");
        return false;
    }

    if (swr_is_initialized(resampleCtx) == 0) {
        if (swr_init(resampleCtx) < 0) {
            utils::errorMsg("Init context failure!");
            return false;
        } 
    }

    return true;
}

bool AudioDecoderLibav::outputConfig()
{
    if (inChannels == 0 && inSampleRate == 0) {
        return true;
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
            utils::errorMsg("Init context failure!");
            return false;
        } 
    }

    return true;
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

void AudioDecoderLibav::checkInputParams(int sampleFormatCode, int channels, int sampleRate)
{
    AVSampleFormat sampleFormat = getAVSampleFormatFromtIntCode(sampleFormatCode);


    if (inLibavSampleFmt == sampleFormat && inChannels == channels && inSampleRate == sampleRate) {
        return;
    }

    inLibavSampleFmt = sampleFormat;
    inChannels = channels;
    inSampleRate = sampleRate;

    switch(inLibavSampleFmt) {
        case AV_SAMPLE_FMT_U8:
            inSampleFmt = U8;
            break;
        case AV_SAMPLE_FMT_S16:
            inSampleFmt = S16;
            break;
        case AV_SAMPLE_FMT_FLT:
            inSampleFmt = FLT;
            break;
        case AV_SAMPLE_FMT_U8P:
            inSampleFmt = U8P;
            break;
        case AV_SAMPLE_FMT_S16P:
            inSampleFmt = S16P;
            break;
        case AV_SAMPLE_FMT_FLTP:
            inSampleFmt = FLTP;
            break;
        default:
            inSampleFmt = S_NONE;
            break;
    }

    inputConfig();
}

void AudioDecoderLibav::configEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    SampleFmt newSampleFmt = outSampleFmt;
    int newChannels = outChannels;
    int newSampleRate = outSampleRate;

    if (!params) {
        outputNode.Add("error", "Error configuring audio decoder");
        return;
    }

    if (params->Has("sampleRate")) {
        newSampleRate = params->Get("sampleRate").ToInt();
    }

    if (params->Has("channels")) {
        newChannels = params->Get("channels").ToInt();
    }

    if (params->Has("sampleFormat")) {
        newSampleFmt = utils::getSampleFormatFromString(params->Get("sampleFormat").ToString());
    }

    if (!configure(newSampleFmt, newChannels, newSampleRate)) {
        outputNode.Add("error", "Error configuring audio decoder");
    } else {
        outputNode.Add("error", Jzon::null);
    }
}

void AudioDecoderLibav::initializeEventMap()
{
    eventMap["configure"] = std::bind(&AudioDecoderLibav::configEvent, this, std::placeholders::_1, std::placeholders::_2);
}

void AudioDecoderLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", outSampleRate);
    filterNode.Add("channels", outChannels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(outSampleFmt));
}

bool AudioDecoderLibav::reconfigureDecoder(AudioFrame* frame)
{
    if (frame->getCodec() == fCodec ) {
        return true;
    }

    fCodec = frame->getCodec();

    switch(fCodec) {
        case PCMU:
            codecID = AV_CODEC_ID_PCM_MULAW;
            break;
        case OPUS:
            codecID = AV_CODEC_ID_OPUS;
            break;
        case AAC:
            codecID = AV_CODEC_ID_AAC;
            break;
        case MP3:
            codecID = AV_CODEC_ID_MP3;
            break;
        default:
            codecID = AV_CODEC_ID_NONE;
            break;
    }

    av_frame_unref(inFrame);

    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }

    codec = avcodec_find_decoder(codecID);
    
    if (codec == NULL) {
        utils::errorMsg("[DECODER] Error finding codec!");
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        utils::errorMsg("[DECODER] Error allocating context!");
        return false;
    }

    codecCtx->channels = inChannels;
    codecCtx->channel_layout = av_get_default_channel_layout(inChannels);
    codecCtx->sample_rate = inSampleRate;
    
    AVDictionary* dictionary = NULL;
    if (avcodec_open2(codecCtx, codec, &dictionary) < 0) {
        utils::errorMsg("[DECODER] Error open context!");
        return false;
    }

    return true;
}






