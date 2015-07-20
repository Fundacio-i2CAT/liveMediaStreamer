/*
 *  HeadDemuxerLibav.cpp - A libav-based head (source) demuxer
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
 *  Authors: Xavi Artigas <xavier.artigas@i2cat.net>  
 */

#include "HeadDemuxerLibav.hh"
#include "../../AVFramedQueue.hh"

HeadDemuxerLibav::HeadDemuxerLibav() : HeadFilter (MASTER, 2)
{
    // Initialize libav
    av_register_all();
    avformat_network_init();

    av_ctx = NULL;

    fType = DEMUXER;

    // Clear all internal data
    reset();
}

HeadDemuxerLibav::~HeadDemuxerLibav()
{
    // Free any possible memory and close possible files/streams
    reset();

    // Shutdown libav
    avformat_network_deinit();
}

void HeadDemuxerLibav::reset()
{
    if (av_ctx) {
        avformat_close_input (&av_ctx);
    }
    av_ctx = NULL;

    // Free extradatas and stream infos
    for (auto sinfo : outputStreamInfos) {
        delete sinfo.second;
    }
    outputStreamInfos.clear();
}

bool HeadDemuxerLibav::doProcessFrame(std::map<int, Frame*> dstFrames)
{
    if (!av_ctx) return false;

    // Ask libav for a data packet
    AVPacket pkt;
    av_init_packet (&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    if (av_read_frame (av_ctx, &pkt) < 0) {
        return false;
    }

    // Find the stream index of the packet and the corresponding output frame
    Frame *f = dstFrames[pkt.stream_index];
    if (f != NULL) {
        // If the output for this stream index is connected, copy payload
        // (Otherwise, discard packet)
        uint8_t *data = f->getDataBuf();
        memcpy (data, pkt.data, pkt.size);
        f->setConsumed(true);
        f->setLength(pkt.size);
        f->setPresentationTime(
            std::chrono::microseconds(
                (int64_t)(1000000 * pkt.pts * streamTimeBase[pkt.stream_index])));
        f->setDuration(
            std::chrono::nanoseconds(
                (int64_t)(1000000000 * pkt.duration * streamTimeBase[pkt.stream_index])));
    }
    av_free_packet(&pkt);

    return true;
}

FrameQueue *HeadDemuxerLibav::allocQueue(struct ConnectionData cData)
{
    // Create output queue for the kind of stream associated with this wId
    const StreamInfo *si = outputStreamInfos[cData.writerId];
    switch (si->type) {
        case AUDIO:
            return AudioFrameQueue::createNew(cData, si, DEFAULT_AUDIO_FRAMES);
        case VIDEO:
            return VideoFrameQueue::createNew(cData, si, DEFAULT_VIDEO_FRAMES);
        default:
            break;
    }
    return NULL;
}

void HeadDemuxerLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("uri", uri);
    Jzon::Array jstreams;
    for (auto it : outputStreamInfos) {
        Jzon::Object s;
        s.Add("wId", it.first);
        s.Add("type", it.second->type);
        switch (it.second->type) {
            case AUDIO:
                s.Add("codec", it.second->audio.codec);
                s.Add("sampleRate", (int)it.second->audio.sampleRate);
                s.Add("channels", (int)it.second->audio.channels);
                s.Add("sampleFormat", it.second->audio.sampleFormat);
                break;
            case VIDEO:
                s.Add("codec", it.second->video.codec);
                break;
            default:
                break;
        }
        jstreams.Add(s);
    }
    filterNode.Add("streams", jstreams);
}

bool HeadDemuxerLibav::setURI(const std::string URI)
{
    // Close previous URI, if there was one, and clear associated data
    reset();

    uri = URI;

    int res;

    // Try to open the URI
    res = avformat_open_input(&av_ctx, uri.c_str(), NULL, NULL);
    if (res < 0) {
        char err[1024];
        av_make_error_string(err, sizeof(err), res);
        utils::errorMsg("avformat_open_input (" + URI + "): " + err);
        return false;
    }

    // Try to recover stream info
    res = avformat_find_stream_info(av_ctx, NULL);
    if (res < 0) {
        char err[1024];
        av_make_error_string(err, sizeof(err), res);
        utils::errorMsg("avformat_find_stream_info (" + URI + "): " + err);
        return false;
    }

    // Build StreamInfos and map them through wId
    for (unsigned int i=0; i<av_ctx->nb_streams; i++) {
        const AVCodecDescriptor* cdesc =
                avcodec_descriptor_get(av_ctx->streams[i]->codec->codec_id);
        StreamInfo *si = new StreamInfo();
        if (cdesc) {
            switch (cdesc->type) {
                case AVMEDIA_TYPE_AUDIO:
                    si->type = AUDIO;
                    si->audio.codec = utils::getAudioCodecFromLibavString(cdesc->name);
                    si->audio.sampleRate = av_ctx->streams[i]->codec->sample_rate;
                    si->audio.channels = av_ctx->streams[i]->codec->channels;
                    si->audio.sampleFormat = getSampleFormatFromLibav (
                            av_ctx->streams[i]->codec->sample_fmt);
                    // Overwrite libav values with our per-codec defaults
                    si->setCodecDefaults();
                    break;
                case AVMEDIA_TYPE_VIDEO:
                    si->type = VIDEO;
                    si->video.codec = utils::getVideoCodecFromLibavString(cdesc->name);
                    // Overwrite libav values with our per-codec defaults
                    si->setCodecDefaults();
                    break;
                default:
                    // Ignore this stream
                    break;
            }
            if (av_ctx->streams[i]->codec->extradata_size > 0) {
                si->setExtraData(av_ctx->streams[i]->codec->extradata,
                        av_ctx->streams[i]->codec->extradata_size);
            }
            double timeBase = (double)av_ctx->streams[i]->time_base.num /
                    av_ctx->streams[i]->time_base.den;
            streamTimeBase[i] = timeBase;
        }
        outputStreamInfos[i] = si;
    }

    return true;
}

SampleFmt HeadDemuxerLibav::getSampleFormatFromLibav(AVSampleFormat libavSampleFmt)
{
    switch (libavSampleFmt) {
        case AV_SAMPLE_FMT_U8: return U8;
        case AV_SAMPLE_FMT_S16: return S16;
        case AV_SAMPLE_FMT_FLT: return FLT;
        case AV_SAMPLE_FMT_U8P: return U8P;
        case AV_SAMPLE_FMT_S16P: return S16P;
        case AV_SAMPLE_FMT_FLTP: return FLTP;
        default: return S_NONE;
    }
}
