/*
 *  VideoEncoderX264 - Video Encoder X264
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
 *  Authors:  Martin German <martin.german@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>
 */

#include <cmath>
#include "VideoEncoderX264.hh"

VideoEncoderX264::VideoEncoderX264(FilterRole fRole_, int framerate, bool shareFrames) : 
OneToOneFilter(1000000/framerate, fRole_ , true, shareFrames)
{
    fType = VIDEO_ENCODER;

    pts = 0;
    fps = framerate;

    forceIntra = false;
    encoder = NULL;
    x264_picture_init(&picIn);
    midFrame = av_frame_alloc();

    configure();

    initializeEventMap();
}

VideoEncoderX264::~VideoEncoderX264(){
	//TODO: delete encoder;
}

bool VideoEncoderX264::doProcessFrame(Frame *org, Frame *dst) {
	VideoFrame* videoFrame = dynamic_cast<VideoFrame*> (org);
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> (dst);
	int frameLength;
	int piNal;
	picIn.i_pts = pts;

    if (!reconfigure(videoFrame, x264Frame)) {
        return false;
    }

    if (forceIntra) {
        picIn.i_type = X264_TYPE_I;
        forceIntra = false;
    } else {
        picIn.i_type = X264_TYPE_AUTO;
    }

    if (!fill_x264_picture(videoFrame)){
        utils::errorMsg("Could not fill x264_picture_t from frame");
        return false;
    }

    frameLength = x264_encoder_encode(encoder, &ppNal, &piNal, &picIn, &picOut);

    if (frameLength < 1) {
        utils::errorMsg("Could not encode video frame");
        return false;
    }

    x264Frame->setNals(&ppNal, piNal, frameLength);
    x264Frame->setSize(videoFrame->getWidth(), videoFrame->getHeight());
    
    pts++;
	return true;
}

bool VideoEncoderX264::fill_x264_picture(VideoFrame* videoFrame)
{
    if (avpicture_fill((AVPicture *) midFrame, videoFrame->getDataBuf(),
            (AVPixelFormat) libavInPixFmt, videoFrame->getWidth(),
            videoFrame->getHeight()) <= 0){
        utils::errorMsg("Could not feed AVFrame");
        return false;
    }
    picIn.img.i_csp = colorspace;

    for(int i = 0; i < 4; i++){
        picIn.img.i_stride[i] = midFrame->linesize[i];
        picIn.img.plane[i] = midFrame->data[i];
    }

    return true;
}

bool VideoEncoderX264::encodeHeadersFrame(X264VideoFrame* x264Frame)
{
	int encodeSize;
    int piNal;
	x264_nal_t *ppNal;

	encodeSize = x264_encoder_headers(encoder, &ppNal, &piNal);

	if (encodeSize < 0) {
		utils::errorMsg("Could not encode headers");
        return false;
	}

	x264Frame->setHeaderNals(&ppNal, piNal, encodeSize);

    return true;
}

FrameQueue* VideoEncoderX264::allocQueue(int wId) {
	return X264VideoCircularBuffer::createNew();
}

bool VideoEncoderX264::configure(int gop_, int bitrate_, int threads_, int fps_, bool annexB_)
{
    //TODO: validate inputs
    gop = gop_;
    bitrate = bitrate_;
    threads = threads_;
    annexB = annexB_;

    if (fps_ == 0) {
        fps = VIDEO_DEFAULT_FRAMERATE;
    } else {
        fps = fps_;
    }

    setFrameTime(std::chrono::nanoseconds(std::nano::den/fps));

    needsConfig = true;
    return true;
}

bool VideoEncoderX264::reconfigure(VideoFrame* orgFrame, X264VideoFrame* x264Frame)
{
    if (needsConfig || orgFrame->getWidth() != xparams.i_width ||
        orgFrame->getHeight() != xparams.i_height ||
        orgFrame->getPixelFormat() != inPixFmt)
    {
        inPixFmt = orgFrame->getPixelFormat();
        switch (inPixFmt) {
            case YUV420P:
                libavInPixFmt = AV_PIX_FMT_YUV420P;
                colorspace = X264_CSP_I420;
                break;
            case YUV422P:
                libavInPixFmt = AV_PIX_FMT_YUV422P;
                colorspace = X264_CSP_I422;
                break;
            case YUV444P:
                libavInPixFmt = AV_PIX_FMT_YUV444P;
                colorspace = X264_CSP_I444;
                break;
            default:
                utils::debugMsg("Uncompatibe input pixel format");
                libavInPixFmt = AV_PIX_FMT_NONE;
                colorspace = X264_CSP_NONE;
                return false;
                break;
        }

        x264_param_default_preset(&xparams, "ultrafast", "zerolatency");
        xparams.i_threads = threads;
        xparams.i_fps_num = fps;
        xparams.i_fps_den = 1;
        xparams.b_intra_refresh = 0;
        xparams.b_aud = 1;
        xparams.i_keyint_max = gop;
        xparams.rc.i_bitrate = bitrate;
        if (annexB){
            xparams.b_annexb = 1;
            xparams.b_repeat_headers = 1;
        }

        if (orgFrame->getWidth() != xparams.i_width ||
            orgFrame->getHeight() != xparams.i_height){
            xparams.i_width = orgFrame->getWidth();
            xparams.i_height = orgFrame->getHeight();
            if (encoder != NULL){
                x264_encoder_close(encoder);
                encoder = x264_encoder_open(&xparams);
                needsConfig = false;
            }
        }
        //TODO: set profile
        x264_param_apply_profile(&xparams, "baseline");


        if (encoder == NULL){
            encoder = x264_encoder_open(&xparams);
        } else if (needsConfig && x264_encoder_reconfig(encoder, &xparams) < 0){
            utils::errorMsg("Could not reconfigure encoder, closing and opening again");
            x264_encoder_close(encoder);
            encoder = x264_encoder_open(&xparams);
        }

        needsConfig = false;

        if (!annexB){
            return encodeHeadersFrame(x264Frame);
        }
    }

    return true;
}

void VideoEncoderX264::configEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int tmpGop, tmpBitrate, tmpThreads;
    bool tmpAnnexB;

    if (!params) {
        return;
    }

    tmpGop = gop;
    tmpBitrate = bitrate;
    tmpThreads = threads;
    tmpAnnexB = annexB;

    if (params->Has("gop")){
        tmpGop = params->Get("gop").ToInt();
    }

    if (params->Has("bitrate")){
        tmpBitrate = params->Get("bitrate").ToInt();
    }

    if (params->Has("threads")){
        tmpThreads = params->Get("threads").ToInt();
    }

    if (params->Has("annexb")){
        tmpAnnexB = params->Get("annexb").ToBool();
    }

    if (!configure(tmpGop, tmpBitrate, tmpThreads, tmpAnnexB)){
        outputNode.Add("error", "Error configuring vide encoder");
    } else {
        outputNode.Add("error", Jzon::null);
    }
}

void VideoEncoderX264::forceIntraEvent(Jzon::Node* params)
{
	forceIntra = true;
}

void VideoEncoderX264::initializeEventMap()
{
	eventMap["forceIntra"] = std::bind(&VideoEncoderX264::forceIntraEvent, this, std::placeholders::_1);
    eventMap["configure"] = std::bind(&VideoEncoderX264::configEvent, this, std::placeholders::_1, std::placeholders::_2);
}

void VideoEncoderX264::doGetState(Jzon::Object &filterNode)
{
    //TODO
}
