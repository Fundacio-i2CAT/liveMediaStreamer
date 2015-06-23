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
//#include "../../AudioFrame.hh"
//#include "../../VideoFrame.hh"

HeadDemuxerLibav::HeadDemuxerLibav() : HeadFilter (MASTER, 0, 2)
{
    av_register_all();
    avformat_network_init();

    av_ctx = NULL;

    reset();
}

HeadDemuxerLibav::~HeadDemuxerLibav()
{
    reset();

    avformat_network_deinit();
}

void HeadDemuxerLibav::reset()
{
    if (av_ctx) {
        avformat_close_input (&av_ctx);
    }
    av_ctx = NULL;
    last_writer_id = 0;
    streams.clear();
}

bool HeadDemuxerLibav::doProcessFrame(std::map<int, Frame*> dstFrames)
{
    return true;
}

FrameQueue *HeadDemuxerLibav::allocQueue(int wId)
{
    switch (streams[wId].type) {
        case AUDIO:
            return AudioFrameQueue::createNew(AC_NONE, DEFAULT_AUDIO_FRAMES);
        case VIDEO:
            return VideoFrameQueue::createNew(VC_NONE, DEFAULT_VIDEO_FRAMES);
        default:
            break;
    }
    return NULL;
}

void HeadDemuxerLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("uri", uri);
    Jzon::Array jstreams;
    for (auto it : streams) {
        Jzon::Object s;
        s.Add("wId", it.first);
        s.Add("type", it.second.type);
        s.Add("codec_name", it.second.codec_name);
        jstreams.Add(s);
    }
    filterNode.Add("streams", jstreams);
}

bool HeadDemuxerLibav::setURI(const std::string URI)
{
    reset();

    uri = URI;

    int res;

    res = avformat_open_input(&av_ctx, uri.c_str(), NULL, NULL);
    if (res < 0) {
        char err[1024];
        av_make_error_string(err, sizeof(err), res);
        std::cerr << "avformat_open_input (" << URI << "): " << err << std::endl;
        return false;
    }

    res = avformat_find_stream_info(av_ctx, NULL);
    if (res < 0) {
        char err[1024];
        av_make_error_string(err, sizeof(err), res);
        std::cerr << "avformat_find_stream_info (" + URI + "): " << err << std::endl;
        return false;
    }

    for (unsigned int i=0; i<av_ctx->nb_streams; i++) {
        const AVCodecDescriptor* cdesc =
                avcodec_descriptor_get(av_ctx->streams[i]->codec->codec_id);
        DemuxerStreamInfo sinfo;
        sinfo.type =
                cdesc->type == AVMEDIA_TYPE_VIDEO ? VIDEO :
                cdesc->type == AVMEDIA_TYPE_AUDIO ? AUDIO :
                ST_NONE;
        sinfo.codec_name = cdesc->name;
        streams[last_writer_id] = sinfo;
        last_writer_id++;
    }

    return true;
}
