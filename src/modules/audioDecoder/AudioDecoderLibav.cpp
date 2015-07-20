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

AudioDecoderLibav::AudioDecoderLibav()
: OneToOneFilter()
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

    configure0(FLTP, DEFAULT_CHANNELS, DEFAULT_SAMPLE_RATE);
}

AudioDecoderLibav::~AudioDecoderLibav()
{
    avcodec_close(codecCtx);
    av_free(codecCtx);
    swr_free(&resampleCtx);
    av_free(inFrame);
    av_free_packet(&pkt);
}

FrameQueue* AudioDecoderLibav::allocQueue(ConnectionData cData)
{
    return AudioCircularBuffer::createNew(cData, outChannels, outSampleRate, DEFAULT_BUFFER_SIZE, 
                                            outSampleFmt, std::chrono::milliseconds(0));
}

bool AudioDecoderLibav::doProcessFrame(Frame *org, Frame *dst)
{
    int len, gotFrame;

    AudioFrame* aCodedFrame = dynamic_cast<AudioFrame*>(org);
    AudioFrame* aDecodedFrame = dynamic_cast<AudioFrame*>(dst);

    if (!reconfigureDecoder(aCodedFrame)) {
        utils::errorMsg("Error reconfiguring decoder: check input frame params");
        return false;
    }

    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();

    if (pkt.size <= 0) {
        utils::errorMsg("Error decoding audio frame: pkt.size <= 0");
        return false;
    }   

    while (pkt.size > 0) {
        len = avcodec_decode_audio4(codecCtx, inFrame, &gotFrame, &pkt);

        if(len < 0) {
            utils::errorMsg("Error decoding audio frame");
            return false;
        }

        if (!gotFrame) {

            if (pkt.data) {
                pkt.size -= len;
                pkt.data += len;
            }

            continue;
        }

        checkSampleFormat(inFrame->format);

        if (!resample(inFrame, aDecodedFrame)) {
            utils::errorMsg("Error resampling audio frame");
            return false;
        }

        dst->setConsumed(true);
        return true;
    }

    dst->setConsumed(true);
    return true;
}

bool AudioDecoderLibav::configure0(SampleFmt sampleFormat, int channels, int sampleRate)
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
        utils::errorMsg("[AudioDecoder::outpuConfig()] Error creating resample context");
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

void AudioDecoderLibav::checkSampleFormat(int sampleFormatCode)
{
    AVSampleFormat sampleFormat = getAVSampleFormatFromtIntCode(sampleFormatCode);

    if (inLibavSampleFmt == sampleFormat) {
        return;
    }

    inLibavSampleFmt = sampleFormat;

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

bool AudioDecoderLibav::configEvent(Jzon::Node* params)
{
    SampleFmt newSampleFmt = outSampleFmt;
    int newChannels = outChannels;
    int newSampleRate = outSampleRate;

    if (!params) {
        return false;
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

    return configure0(newSampleFmt, newChannels, newSampleRate);
}

bool AudioDecoderLibav::configure(SampleFmt sampleFormat, int channels, int sampleRate)
{
    Jzon::Object root, params;
    root.Add("action", "configure");
    params.Add("sampleFormat", utils::getSampleFormatAsString(sampleFormat));
    params.Add("channels", channels);
    params.Add("sampleRate", sampleRate);
    root.Add("params", params);

    Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}

void AudioDecoderLibav::initializeEventMap()
{
    eventMap["configure"] = std::bind(&AudioDecoderLibav::configEvent, this, std::placeholders::_1);
}

void AudioDecoderLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", (int)outSampleRate);
    filterNode.Add("channels", (int)outChannels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(outSampleFmt));
}

bool AudioDecoderLibav::reconfigureDecoder(AudioFrame* frame)
{
    AVCodecID codecId;

    if (frame->getChannels() <= 0 || frame->getSampleRate() <= 0) {
        utils::errorMsg("Error reconfiguring audio decoder: input channels or sample rate values not valid");
        return false;
    }

    if (frame->getCodec() == fCodec && frame->getChannels() == inChannels && frame->getSampleRate() == inSampleRate) {
        return true;
    }

    fCodec = frame->getCodec();
    inChannels = frame->getChannels();
    inSampleRate = frame->getSampleRate();

    switch(fCodec) {
        case PCMU:
            codecId = AV_CODEC_ID_PCM_MULAW;
            break;
        case OPUS:
            codecId = AV_CODEC_ID_OPUS;
            break;
        case AAC:
            codecId = AV_CODEC_ID_AAC;
            break;
        case MP3:
            codecId = AV_CODEC_ID_MP3;
            break;
        default:
            codecId = AV_CODEC_ID_NONE;
            break;
    }

    if (codecId == AV_CODEC_ID_NONE) {
        utils::errorMsg("Error reconfiguring audio decoder: input codec not supported");
        return false;
    }

    av_frame_unref(inFrame);

    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }

    codec = avcodec_find_decoder(codecId);

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
