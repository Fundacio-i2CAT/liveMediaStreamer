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

VideoEncoderX264::VideoEncoderX264(FilterRole fRole, bool sharedFrames, int framerate) :
VideoEncoderX264or5(fRole, sharedFrames, framerate), encoder(NULL)
{
    pts = 0;
    encoder = NULL;
    x264_picture_init(&picIn);
    x264_picture_init(&picOut);
}

VideoEncoderX264::~VideoEncoderX264()
{
	// x264_picture_clean(picIn);
}

bool VideoEncoderX264::fillPicturePlanes(unsigned char** data, int* linesize)
{
    if (&picIn.img == NULL) {
        return false;
    }

    for(int i = 0; i < MAX_PLANES_PER_PICTURE; i++){
        picIn.img.i_stride[i] = linesize[i];
        picIn.img.plane[i] = data[i];
    }

    return true;
}

bool VideoEncoderX264::encodeFrame(VideoFrame* codedFrame)
{
    int frameLength;
    int piNal;

    X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> (codedFrame);

    if (!x264Frame || !encoder) {
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
    frameLength = x264_encoder_encode(encoder, &ppNal, &piNal, &picIn, &picOut);

    if (frameLength < 1) {
        utils::errorMsg("Could not encode video frame");
        return false;
    }

    pts++;
    x264Frame->setNals(&ppNal, piNal, frameLength);
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

FrameQueue* VideoEncoderX264::allocQueue(int wId) 
{
	return X264VideoCircularBuffer::createNew();
}

bool VideoEncoderX264::reconfigure(VideoFrame* orgFrame, VideoFrame* dstFrame)
{   
    int colorspace;
    X264VideoFrame* x264Frame;

    if (!needsConfig && orgFrame->getWidth() == xparams.i_width &&
        orgFrame->getHeight() == xparams.i_height && orgFrame->getPixelFormat() == inPixFmt) {
        return true;
    }

    x264Frame = dynamic_cast<X264VideoFrame*> (dstFrame);

    if (!x264Frame) {
        utils::errorMsg("Error reconfiguring x264VideoEncoder. DstFrame MUST be a x264VideoFrame");
        return false;
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
            utils::debugMsg("Uncompatibe input pixel format");
            libavInPixFmt = AV_PIX_FMT_NONE;
            colorspace = X264_CSP_NONE;
            return false;
            break;
    }

    picIn.img.i_csp = colorspace;
    x264_param_default_preset(&xparams, preset.c_str(), NULL);
    x264_param_apply_profile(&xparams, "high");

    xparams.i_threads = threads;
    xparams.i_fps_num = fps;
    xparams.i_fps_den = 1;
    xparams.b_intra_refresh = 0;
    xparams.b_aud = 1;
    xparams.i_keyint_max = gop;
    xparams.rc.i_bitrate = bitrate;
    xparams.b_repeat_headers = 0;
    xparams.i_bframe = 0;

    if (annexB) {
        xparams.b_annexb = 1;
        xparams.b_repeat_headers = 1;
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
    
    if (!annexB) {
        return encodeHeadersFrame(x264Frame);
    }
    
    return true;
}