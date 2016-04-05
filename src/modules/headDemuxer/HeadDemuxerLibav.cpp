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

HeadDemuxerLibav::HeadDemuxerLibav() : HeadFilter ()
{
    // Initialize libav
    av_register_all();
    avformat_network_init();

    av_ctx = NULL;
    av_filter_annexb = NULL;

    av_pkt.data = NULL;
    buffer = NULL;

    fType = DEMUXER;

    // Clear all internal data
    reset();

    initializeEventMap();
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

    if (av_filter_annexb) {
        av_bitstream_filter_close(av_filter_annexb);
    }
    av_filter_annexb = NULL;

    if (buffer && buffer != av_pkt.data) {
        free(buffer);
    }
    buffer = NULL;

    if (av_pkt.data) {
        av_packet_unref(&av_pkt);
    }

    // Free stream infos
    for (auto sinfo : outputStreamInfos) {
        delete sinfo.second;
    }
    outputStreamInfos.clear();

    // Free private stream infos
    for (auto sinfo : privateStreamInfos) {
        delete sinfo.second;
    }
    privateStreamInfos.clear();
}

static int findStartCode(uint8_t *buffer, int offs, int buffer_length)
{
    while (offs < buffer_length - 3) {
        if (buffer[offs + 0] == 0 && buffer[offs + 1] == 0) {
            if (buffer[offs + 2] == 1) {
                return offs;
            }
            if (buffer[offs + 2] == 0 && buffer[offs + 3] == 1) {
                return offs;
            }
        }
        offs++;
    }
    return -1;
}

bool HeadDemuxerLibav::doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret)
{
    PrivateStreamInfo *psi;

    if (!av_ctx) return false;

    if (!av_pkt.data) {
        // Ask libav for a new data packet
        av_init_packet (&av_pkt);
        av_pkt.data = NULL;
        av_pkt.size = 0;
        if (av_read_frame (av_ctx, &av_pkt) < 0) {
            return false;
        }
        if (privateStreamInfos.count(av_pkt.stream_index) == 0) {
            // Unknown stream? weird...
            av_packet_unref(&av_pkt);
            return false;
        }
        bufferOffset = 0;
        psi = privateStreamInfos[av_pkt.stream_index];
        if (av_pkt.pts != AV_NOPTS_VALUE){
            psi->lastPTS = av_pkt.pts;
        }
        if (psi->needsFraming && !psi->isAnnexB) {
            // Convert to Annex B (adding startcodes) using temp buffer
            int res;
            res = av_bitstream_filter_filter (av_filter_annexb,
                    av_ctx->streams[av_pkt.stream_index]->codec,
                    NULL, &buffer, &bufferSize, av_pkt.data,
                    av_pkt.size, av_pkt.flags & AV_PKT_FLAG_KEY);
            if (res < 0) {
                av_packet_unref(&av_pkt);
                buffer = 0;
                return false;
            }
            if (res == 0 && buffer != av_pkt.data) {
                // No buffer has been allocated, take some precautions so we do not
                // try to free it later on
                bufferOffset = buffer - av_pkt.data;
                buffer = av_pkt.data;
            }
        } else {
            // Processing the packet directly
            buffer = av_pkt.data;
            bufferSize = av_pkt.size;
        }
    } else {
        psi = privateStreamInfos[av_pkt.stream_index];
    }

    if (dstFrames.count(av_pkt.stream_index) == 0) {
        // Discard packet if output corresponding to this stream is not connected
        if (buffer != av_pkt.data) {
            free(buffer);
        }
        buffer = NULL;
        av_packet_unref(&av_pkt);
        return false;
    }

    // Find corresponding output frame
    Frame *f = dstFrames[av_pkt.stream_index];

    // Copy to destination frame, framing if necessary
    uint8_t *dst_data = f->getDataBuf();
    int dst_size = bufferSize;
    if (psi->needsFraming) {
        // Split on startcode boundaries
        int start = findStartCode(buffer, bufferOffset, bufferSize);
        if (start == -1) {
            utils::errorMsg("Malformed Annex B stream (could not find start code)");
        }

        int end = findStartCode(buffer, bufferOffset + 4, bufferSize - 4);
        bufferOffset = end;
        if (end == -1) {
            end = bufferSize;
        }
        dst_size = end - start;

        // LMS does not like short startcodes, so detect them and turn into long startcodes
        if (buffer[start + 2] == 1) {
            dst_data[0] = 0;
            dst_data++;
        }
        memcpy(dst_data, buffer + start, dst_size);
        if (buffer[start + 2] == 1) dst_size++;

    } else {
        // No conversion needed, just copy
        memcpy (dst_data, buffer, bufferSize);
        bufferOffset = -1;
    }
    f->setConsumed(true);
    f->setLength(dst_size);
        
    if (av_pkt.pts == AV_NOPTS_VALUE) {
        f->setPresentationTime(
            std::chrono::microseconds(psi->lastPTS + av_ctx->start_time_realtime));
    } else {
        f->setPresentationTime(
            std::chrono::microseconds(
                (int64_t)(av_pkt.pts * psi->streamTimeBase * std::micro::den) + av_ctx->start_time_realtime));
    }

    if (bufferOffset == -1) {
        if (buffer != av_pkt.data) {
            free(buffer);
        }
        buffer = NULL;
        av_packet_unref(&av_pkt);
    }

    return true;
}

FrameQueue *HeadDemuxerLibav::allocQueue(ConnectionData cData)
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
        PrivateStreamInfo *psi = new PrivateStreamInfo();
        memset(psi, 0, sizeof(PrivateStreamInfo));
        psi->lastPTS = 0;
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
                    // For now, we assume H265 is always in AnnexB format
                    si->video.h264or5.annexb = true;
                    break;
                default:
                    // Ignore this stream
                    break;
            }
            if (av_ctx->streams[i]->codec->extradata_size > 0) {
                si->setExtraData(av_ctx->streams[i]->codec->extradata,
                        av_ctx->streams[i]->codec->extradata_size);
                // Detect H264 AnnexB format
                if (av_ctx->streams[i]->codec->extradata_size > 4 &&
                        si->type == VIDEO && si->video.codec == H264) {
                    // We will always parse H264 streams since libav gives us packets containing
                    // a single frame, which might include more than one NALU.
                    psi->needsFraming = true;
                    const uint8_t *data = av_ctx->streams[i]->codec->extradata;
                    // First byte of AVCC extradata is always 1 (version)
                    // AnnexB extradata starts with either 0x000001 or 0x00000001
                    psi->isAnnexB = (data[0] == 0);
                    if (!psi->isAnnexB) {
                        // This is AVCC, we need conversion
                        av_filter_annexb = av_bitstream_filter_init ("h264_mp4toannexb");
                        if (av_filter_annexb == NULL) {
                            utils::warningMsg("Could not find suitable libav filter to convert stream to Annexb");
                            // We actually need framing, but we have no filter
                            psi->needsFraming = false;
                        }
                    }
                    // Always report AnnexB, framed format, since we do all conversions
                    si->video.h264or5.annexb = true;
                    si->video.h264or5.framed = true;
                }
            }
            double timeBase = (double)av_ctx->streams[i]->time_base.num /
                    av_ctx->streams[i]->time_base.den;
            psi->streamTimeBase = timeBase;
        }
        utils::infoMsg("Found stream " + std::to_string(i) + ": " + utils::getStreamInfoAsString(si));
        outputStreamInfos[i] = si;
        privateStreamInfos[i] = psi;
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

void HeadDemuxerLibav::initializeEventMap()
{
    eventMap["configure"] = std::bind(&HeadDemuxerLibav::configureEvent, this, std::placeholders::_1);
}

bool HeadDemuxerLibav::configureEvent(Jzon::Node* params)
{
    if (!params) {
        return false;
    }

    if (params->Has("uri")) {
        if(setURI(params->Get("uri").ToString())){
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}
