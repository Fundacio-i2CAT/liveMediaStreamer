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
#include "../../SlicedVideoFrameQueue.hh"

#define MAX_PLANES_PER_PICTURE 4

VideoEncoderX264::VideoEncoderX264() :
VideoEncoderX264or5(), encoder(NULL)
{
    pts = 0;
    outputStreamInfo->video.codec = H264;
    x264_picture_init(&picIn);
    x264_picture_init(&picOut);
}

VideoEncoderX264::~VideoEncoderX264()
{
    if (encoder != NULL){
        x264_encoder_close(encoder);
        encoder = NULL;
    }
}

bool VideoEncoderX264::fillPicturePlanes(unsigned char** data, int* linesize)
{
    for(int i = 0; i < MAX_PLANES_PER_PICTURE; i++){
        picIn.img.i_stride[i] = linesize[i];
        picIn.img.plane[i] = data[i];
    }

    return true;
}

bool VideoEncoderX264::encodeFrame(VideoFrame* codedFrame)
{
    int success;
    int piNal;
    x264_nal_t* nals;
    SlicedVideoFrame* slicedFrame;

    slicedFrame = dynamic_cast<SlicedVideoFrame*> (codedFrame);

    if (!slicedFrame || !encoder) {
        utils::errorMsg("Could not encode x264 video frame. Target frame or encoder are NULL");
        return false;
    }

    if (forceIntra) {
        picIn.i_type = X264_TYPE_I;
        forceIntra = false;
    } else {
        picIn.i_type = X264_TYPE_AUTO;
    }

    picIn.i_pts = pts;
    success = x264_encoder_encode(encoder, &nals, &piNal, &picIn, &picOut);

    pts++;

    if (success == 0) {
        return false;
    } else if (success < 0) {
        utils::errorMsg("X264 Encoder: Could not encode video frame");
        return false;
    }

    for (int i = 0; i < piNal; i++) {

        if (!slicedFrame->setSlice(nals[i].p_payload, nals[i].i_payload)) {
            utils::errorMsg("X264 Encoder: too many NALs for one slicedFrame");
            return false;
        }
    }

    return true;
}

bool VideoEncoderX264::encodeHeadersFrame()
{
    int encodeSize;
    int piNal;
    x264_nal_t* nals;

    encodeSize = x264_encoder_headers(encoder, &nals, &piNal);

    if (encodeSize < 0) {
        utils::errorMsg("Could not encode headers");
        return false;
    }
       
    outputStreamInfo->setExtraData(nals[0].p_payload, encodeSize);

    return true;
}

FrameQueue* VideoEncoderX264::allocQueue(ConnectionData cData)
{
    return SlicedVideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_VIDEO_FRAMES, MAX_H264_OR_5_NAL_SIZE);
}

bool VideoEncoderX264::reconfigure(VideoFrame* orgFrame, VideoFrame* dstFrame)
{
    int colorspace;

    if (!needsConfig && orgFrame->getWidth() == xparams.i_width &&
        orgFrame->getHeight() == xparams.i_height && orgFrame->getPixelFormat() == inPixFmt) {
        return true;
    }

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
            utils::errorMsg("Uncompatibe input pixel format");
            libavInPixFmt = AV_PIX_FMT_NONE;
            colorspace = X264_CSP_NONE;
            return false;
            break;
    }

    picIn.img.i_csp = colorspace;
    x264_param_default_preset(&xparams, preset.c_str(), NULL);
    x264_param_apply_profile(&xparams, "high");

    x264_param_parse(&xparams, "keyint", std::to_string(gop).c_str());
    x264_param_parse(&xparams, "fps", std::to_string(fps).c_str());
    x264_param_parse(&xparams, "intra-refresh", std::to_string(0).c_str());
    x264_param_parse(&xparams, "threads", std::to_string(threads).c_str());
    x264_param_parse(&xparams, "aud", std::to_string(0).c_str());
    x264_param_parse(&xparams, "bitrate", std::to_string(bitrate).c_str());
    x264_param_parse(&xparams, "bframes", std::to_string(0).c_str());
    x264_param_parse(&xparams, "repeat-headers", std::to_string(0).c_str());
    x264_param_parse(&xparams, "vbv-maxrate", std::to_string(bitrate*1.05).c_str());
    x264_param_parse(&xparams, "vbv-bufsize", std::to_string(bitrate*2).c_str());
    x264_param_parse(&xparams, "rc-lookahead", std::to_string(lookahead).c_str());
    x264_param_parse(&xparams, "scenecut", std::to_string(0).c_str());

    if (outputStreamInfo->video.h264or5.annexb) {
        x264_param_parse(&xparams, "repeat-headers", std::to_string(1).c_str());
        x264_param_parse(&xparams, "annexb", std::to_string(1).c_str());
    }

    if (orgFrame->getWidth() != xparams.i_width || orgFrame->getHeight() != xparams.i_height) {
        xparams.i_width = orgFrame->getWidth();
        xparams.i_height = orgFrame->getHeight();

        if (encoder != NULL){
            x264_encoder_close(encoder);
            encoder = NULL;
        }
    }

    if (!encoder) {
        encoder = x264_encoder_open(&xparams);
    } else if (x264_encoder_reconfig(encoder, &xparams) < 0) {
        utils::errorMsg("Could not reconfigure x264 encoder, closing and opening again");
        x264_encoder_close(encoder);
        encoder = x264_encoder_open(&xparams);
    }

    if (!encoder) {
        utils::errorMsg("Error reconfiguring x264 encoder. At this point encoder should not be NULL...");
        return false;
    }

    needsConfig = false;
   
    return encodeHeadersFrame();

}
