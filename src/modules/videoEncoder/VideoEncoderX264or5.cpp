/*
 *  VideoEncoderX264or5 - Base class for VideoEncoderX264 and VideoEncoderX265
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *            David Cassany <david.cassany@i2cat.net>
 */

#include "VideoEncoderX264or5.hh"

VideoEncoderX264or5::VideoEncoderX264or5(FilterRole fRole_) :
OneToOneFilter(fRole_), inPixFmt(P_NONE), forceIntra(false), fps(0), bitrate(0), gop(0), threads(0), needsConfig(false)
{
    fType = VIDEO_ENCODER;
    midFrame = av_frame_alloc();
    outputStreamInfo = new StreamInfo(VIDEO);
    outputStreamInfo->video.h264or5.annexb = false;
    initializeEventMap();
    configure0(DEFAULT_BITRATE, VIDEO_DEFAULT_FRAMERATE, DEFAULT_GOP, 
              DEFAULT_LOOKAHEAD, DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET);
}

VideoEncoderX264or5::~VideoEncoderX264or5()
{
    if (midFrame){
        av_frame_free(&midFrame);
    }
}

bool VideoEncoderX264or5::doProcessFrame(Frame *org, Frame *dst)
{
    if (!(org && dst)) {
        utils::errorMsg("Error encoding video frame: org or dst are NULL");
        return false;
    }

    VideoFrame* rawFrame = dynamic_cast<VideoFrame*> (org);
    VideoFrame* codedFrame = dynamic_cast<VideoFrame*> (dst);

    if (!rawFrame || !codedFrame) {
        utils::errorMsg("Error encoding video frame: org and dst MUST be VideoFrame");
        return false;
    }

    if (!reconfigure(rawFrame, codedFrame)) {
        utils::errorMsg("Error encoding video frame: reconfigure failed");
        return false;
    }

    if (!fill_x264or5_picture(rawFrame)){
        utils::errorMsg("Could not fill x264_picture_t from frame");
        return false;
    }

    if (!encodeFrame(codedFrame)) {
        utils::errorMsg("Could not encode video frame");
        return false;
    }

    codedFrame->setSize(rawFrame->getWidth(), rawFrame->getHeight());

    dst->setConsumed(true);
    return true;
}

bool VideoEncoderX264or5::fill_x264or5_picture(VideoFrame* videoFrame)
{
    if (avpicture_fill((AVPicture *) midFrame, videoFrame->getDataBuf(),
            (AVPixelFormat) libavInPixFmt, videoFrame->getWidth(),
            videoFrame->getHeight()) <= 0){
        utils::errorMsg("Could not feed AVFrame");
        return false;
    }

    if (!fillPicturePlanes(midFrame->data, midFrame->linesize)) {
        utils::errorMsg("Could not fill picture planes");
        return false;
    }

    return true;
}

bool VideoEncoderX264or5::configure0(int bitrate_, int fps_, int gop_, int lookahead_, int threads_, bool annexB_, std::string preset_)
{
    if (bitrate_ <= 0 || gop_ <= 0 || lookahead_ < 0 || threads_ <= 0 || preset_.empty()) {
        utils::errorMsg("Error configuring VideoEncoderX264or5: invalid configuration values");
        return false;
    }

    bitrate = bitrate_;
    gop = gop_;
    lookahead = lookahead_;
    threads = threads_;

    outputStreamInfo->video.h264or5.annexb = annexB_;
    preset = preset_;

    if (fps_ <= 0) {
        fps = VIDEO_DEFAULT_FRAMERATE;
        setFrameTime(std::chrono::microseconds(0));
    } else {
        fps = fps_;
        setFrameTime(std::chrono::microseconds(std::micro::den/fps));
    }

    needsConfig = true;
    return true;
}

bool VideoEncoderX264or5::configEvent(Jzon::Node* params)
{
    int tmpBitrate;
    int tmpFps;
    int tmpGop;
    int tmpLookahead;
    int tmpThreads;
    bool tmpAnnexB;
    std::string tmpPreset;

    if (!params) {
        return false;
    }

    tmpBitrate = bitrate;
    tmpFps = fps;
    tmpGop = gop;
    tmpLookahead = lookahead;
    tmpThreads = threads;
    tmpAnnexB = outputStreamInfo->video.h264or5.annexb;
    tmpPreset = preset;

    if (params->Has("bitrate")) {
        tmpBitrate = params->Get("bitrate").ToInt();
    }

    if (params->Has("fps")) {
        tmpFps = params->Get("fps").ToInt();
    }

    if (params->Has("gop")) {
        tmpGop = params->Get("gop").ToInt();
    }

    if (params->Has("lookahead")) {
        tmpLookahead = params->Get("lookahead").ToInt();
    }

    if (params->Has("threads")) {
        tmpThreads = params->Get("threads").ToInt();
    }

    if (params->Has("annexb")) {
        tmpAnnexB = params->Get("annexb").ToBool();
    }

    if (params->Has("preset")) {
        tmpPreset = params->Get("preset").ToString();
    }

    return configure0(tmpBitrate, tmpFps, tmpGop, tmpLookahead, tmpThreads, tmpAnnexB, tmpPreset);
}

bool VideoEncoderX264or5::forceIntraEvent(Jzon::Node*)
{
    forceIntra = true;
    return true;
}

void VideoEncoderX264or5::initializeEventMap()
{
    eventMap["forceIntra"] = std::bind(&VideoEncoderX264or5::forceIntraEvent, this, std::placeholders::_1);
    eventMap["configure"] = std::bind(&VideoEncoderX264or5::configEvent, this, std::placeholders::_1);
}

void VideoEncoderX264or5::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("bitrate", std::to_string(bitrate));
    filterNode.Add("fps", std::to_string(fps));
    filterNode.Add("gop", std::to_string(gop));
    filterNode.Add("lookahead", std::to_string(lookahead));
    filterNode.Add("threads", std::to_string(threads));
    filterNode.Add("annexb", std::to_string(outputStreamInfo->video.h264or5.annexb));
    filterNode.Add("preset", preset);
}

bool VideoEncoderX264or5::configure(int bitrate, int fps, int gop, int lookahead, int threads, bool annexB, std::string preset)
{
    Jzon::Object root, params;
    root.Add("action", "configure");
    params.Add("bitrate", bitrate);
    params.Add("fps", fps);
    params.Add("gop", gop);
    params.Add("lookahead", lookahead);
    params.Add("threads", threads);
    params.Add("annexb", annexB);
    params.Add("preset", preset);
    root.Add("params", params);

    Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}

