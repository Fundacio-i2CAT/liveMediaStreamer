/*
 *  VideoEncoderX265 - Video Encoder X265
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
 *			  Gerard Castillo <gerard.castillo@i2cat.net>
 */

#include <cmath>
#include "VideoEncoderX265.hh"
#include "../../SlicedVideoFrameQueue.hh"

#define MAX_PLANES_PER_PICTURE 3

VideoEncoderX265::VideoEncoderX265() :
VideoEncoderX264or5(), encoder(NULL)
{
    outputStreamInfo->video.codec = H265;
    xparams = x265_param_alloc();
    picIn = x265_picture_alloc();
    picOut = x265_picture_alloc();
}

VideoEncoderX265::~VideoEncoderX265()
{
    if (encoder != NULL){
        x265_encoder_close(encoder);
        encoder = NULL;
    }
    //TODO check for x265_cleanup()?
    //TODO add x265_picture_clean(&x265pic) or x265_picture_free(&x265pic)?
}

bool VideoEncoderX265::fillPicturePlanes(unsigned char** data, int* linesize)
{
    for(int i = 0; i < MAX_PLANES_PER_PICTURE; i++){
        picIn->stride[i] = linesize[i];
        picIn->planes[i] = data[i];
    }

    return true;
}

bool VideoEncoderX265::encodeFrame(VideoFrame* codedFrame)
{
    int success;
    unsigned piNal;
    x265_nal* nals;
    SlicedVideoFrame* slicedFrame;

    slicedFrame = dynamic_cast<SlicedVideoFrame*> (codedFrame);

    if (!slicedFrame || !encoder) {
        utils::errorMsg("Could not encode x265 video frame. Target frame or encoder are NULL");
        return false;
    }

    if (forceIntra) {
        picIn->sliceType = X265_TYPE_IDR;
        forceIntra = false;
    } else {
        picIn->sliceType = X265_TYPE_AUTO;
    }

    picIn->pts = inPts;
    success = x265_encoder_encode(encoder, &nals, &piNal, picIn, picOut);

    if (success < 0) {
        utils::errorMsg("X265 Encoder: Could not encode video frame");
        return false;
    } 
    
    inPts++;
    
    if (success == 0) {
        utils::debugMsg("X265 Encoder: NAL not retrieved after encoding");
        return false;
    }

    outPts = picOut->pts;
    dts = picOut->dts;
    
    for (unsigned i = 0; i < piNal; i++) {
        if (!slicedFrame->setSlice(nals[i].payload, nals[i].sizeBytes)) {
            utils::errorMsg("X265 Encoder: too many NALs for one slicedFrame");
            return false;
        }
    }

    return true;
}

bool VideoEncoderX265::encodeHeadersFrame()
{
    int encodeSize;
    unsigned piNal;
    x265_nal* nals;

    encodeSize = x265_encoder_headers(encoder, &nals, &piNal);

    if (encodeSize < 0) {
        utils::errorMsg("Could not encode headers");
        return false;
    }
       
    outputStreamInfo->setExtraData(nals[0].payload, encodeSize);
        
    return true;
}

FrameQueue* VideoEncoderX265::allocQueue(ConnectionData cData)
{
    return SlicedVideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_VIDEO_FRAMES, MAX_H264_OR_5_NAL_SIZE);
}

bool VideoEncoderX265::reconfigure(VideoFrame* orgFrame, VideoFrame* dstFrame)
{
    int colorspace;

    if (!needsConfig && orgFrame->getWidth() == xparams->sourceWidth &&
        orgFrame->getHeight() == xparams->sourceHeight && orgFrame->getPixelFormat() == inPixFmt) {
        return true;
    }

    inPixFmt = orgFrame->getPixelFormat();
    switch (inPixFmt) {
        case YUV420P:
            libavInPixFmt = AV_PIX_FMT_YUV420P;
            colorspace = X265_CSP_I420;
            break;
        /*TODO X265_CSP_I422 not supported yet. Continue checking x265 library releases for its support.
        case YUV422P:
            libavInPixFmt = AV_PIX_FMT_YUV422P;
            colorspace = X265_CSP_I422;
            break;*/
        case YUV444P:
            libavInPixFmt = AV_PIX_FMT_YUV444P;
            colorspace = X265_CSP_I444;
            break;
        default:
            utils::debugMsg("Uncompatibe input pixel format");
            libavInPixFmt = AV_PIX_FMT_NONE;
            /*TODO X265_CSP_NONE is not implemented. Continue checking x265 library releases for its support*/
            colorspace = -1;
            return false;
            break;
    }

    picIn->colorSpace = colorspace;
    x265_param_default_preset(xparams, preset.c_str(), NULL);
    /*TODO check with NULL profile*/
    x265_param_apply_profile(xparams, "main");

    x265_param_parse(xparams, "keyint", std::to_string(gop).c_str());
    x265_param_parse(xparams, "fps", std::to_string(fps).c_str());
    x265_param_parse(xparams, "input-res", (std::to_string(orgFrame->getWidth()) + 'x' + std::to_string(orgFrame->getHeight())).c_str());

    //TODO check same management for intra-refresh like x264
    //x265_param_parse(xparams, "intra-refresh", std::to_string(0).c_str());

    x265_param_parse(xparams, "frame-threads", std::to_string(threads).c_str());
    x265_param_parse(xparams, "aud", std::to_string(1).c_str());
    x265_param_parse(xparams, "bitrate", std::to_string(bitrate).c_str());
    x265_param_parse(xparams, "bframes", std::to_string(bFrames).c_str());
    x265_param_parse(xparams, "repeat-headers", std::to_string(0).c_str());
    x265_param_parse(xparams, "vbv-maxrate", std::to_string(bitrate*1.05).c_str());
    x265_param_parse(xparams, "vbv-bufsize", std::to_string(bitrate*2).c_str());
    x265_param_parse(xparams, "rc-lookahead", std::to_string(lookahead).c_str());
    x265_param_parse(xparams, "annexb", std::to_string(1).c_str());
    x265_param_parse(xparams, "open-gop", std::to_string(0).c_str());

    if (outputStreamInfo->video.h264or5.annexb) {
        x265_param_parse(xparams, "repeat-headers", std::to_string(1).c_str());
    }

    if (!encoder) {
        encoder = x265_encoder_open(xparams);
    } else {
        /*TODO reimplement it when a reconfigure method appear*/
        x265_encoder_close(encoder);
        encoder = x265_encoder_open(xparams);
    }

    if (!encoder) {
        utils::errorMsg("Error reconfiguring x265 encoder. At this point encoder should not be NULL...");
        return false;
    }

    x265_picture_init(xparams, picIn);
    x265_picture_init(xparams, picOut);

    needsConfig = false;

    return encodeHeadersFrame();
}
