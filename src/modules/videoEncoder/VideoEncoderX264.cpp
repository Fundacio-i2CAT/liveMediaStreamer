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
 */

#include "VideoEncoderX264.hh"

VideoEncoderX264::VideoEncoderX264(): OneToOneFilter(){
	fps = 24;
	gop = 24;
	pts = 0;
	forceIntra = false;
	firstTime = true;
	swsCtx = NULL;
	encoder = NULL;
	bitrate = 2000;
	x264_param_default_preset(&xparams, "ultrafast", "zerolatency");
	xparams.i_threads = 4;
	xparams.i_width = DEFAULT_WIDTH;
	xparams.i_height = DEFAULT_HEIGHT;
	xparams.i_fps_num = fps;
	xparams.i_fps_den = 1;
	xparams.b_intra_refresh = 0;
	xparams.i_keyint_max = gop;
	xparams.rc.i_bitrate = bitrate;
	x264_param_apply_profile(&xparams, "baseline");
}

VideoEncoderX264::~VideoEncoderX264(){
	//delete encoder;
}

bool VideoEncoderX264::doProcessFrame(Frame *org, Frame *dst) {
	InterleavedVideoFrame* videoFrame = dynamic_cast<InterleavedVideoFrame*> (org);
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> (dst);
	int inFps, frameLength, length; 
	x264_nal_t *ppNal;
	int piNal;
	AVFrame *outFrame;

	picIn.i_pts = pts;
	
	if (firstTime) {
		config(org, dst);
		encodeHeadersFrame(org, dst);
		firstTime = false;
		return true;
	}

	if (forceIntra) {
		picIn.i_type = X264_TYPE_I;
		forceIntra = false;
	}
	else
		picIn.i_type = X264_TYPE_AUTO;

	outFrame = av_frame_alloc();
	length = avpicture_fill((AVPicture *) outFrame, videoFrame->getDataBuf(), inPixel, inWidth, inHeight);

    if (length <= 0){
        return false;
    }

	sws_scale(swsCtx, (uint8_t const * const *) outFrame->data, outFrame->linesize, 0, inHeight, picIn.img.plane, picIn.img.i_stride);

	frameLength = x264_encoder_encode(encoder, &ppNal, &piNal, &picIn, &picOut);

	if (frameLength < 1) {
		printf("Error encoding video frame\n");
		return false;
	}

	x264Frame->setNals(&ppNal, piNal, frameLength);

	pts++;
	return true;
}


void VideoEncoderX264::encodeHeadersFrame(Frame *decodedFrame, Frame *encodedFrame) {
	InterleavedVideoFrame* videoFrame = dynamic_cast<InterleavedVideoFrame*> (decodedFrame);
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> (encodedFrame);
	int inFps, encodeSize;
	x264_nal_t *ppNal;
	int piNal;
	
	swsCtx = sws_getContext(inWidth, inHeight, inPixel, outWidth, outHeight, outPixel, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	xparams.i_width = outWidth;
	xparams.i_height = outHeight;
	x264_param_apply_profile(&xparams, "baseline");
	encoder = x264_encoder_open(&xparams);
	x264_picture_alloc(&picIn, colorspace, outWidth, outHeight);
	
	encodeSize = x264_encoder_headers(encoder, &ppNal, &piNal);
	if (encodeSize < 0) {
		printf("Error: encoder headers\n");
	}
	x264Frame->setNals(&ppNal, piNal, encodeSize);
}

FrameQueue* VideoEncoderX264::allocQueue(int wId) {
	return X264VideoCircularBuffer::createNew();
}

bool VideoEncoderX264::config(Frame *org, Frame *dst) {
	InterleavedVideoFrame* videoFrame = dynamic_cast<InterleavedVideoFrame*> (org);
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> (dst);
	PixType inPixelType, outPixelType;
	inWidth = videoFrame->getWidth();
	inHeight = videoFrame->getHeight();
	outWidth = x264Frame->getWidth();
	outHeight = x264Frame->getHeight();
	inPixelType = videoFrame->getPixelFormat();
	outPixelType = x264Frame->getPixelFormat();

	switch (inPixelType) {
		case P_NONE:
			inPixel = AV_PIX_FMT_NONE;
			break;
		case RGB24:
			inPixel = PIX_FMT_RGB24;
			break;
		case RGB32:
			inPixel = PIX_FMT_RGBA;
			break;
		case YUYV422:
			inPixel = PIX_FMT_YUV420P;
			break;
	}

	switch (outPixelType) {
		case P_NONE:
			outPixel = AV_PIX_FMT_NONE;
			colorspace = X264_CSP_NONE;
			break;
		case RGB24:
			outPixel = PIX_FMT_RGB24;
			colorspace = X264_CSP_RGB;
			break;
		case RGB32:
			outPixel = PIX_FMT_RGBA;
			colorspace = X264_CSP_BGRA;
			break;
		case YUYV422:
			outPixel = PIX_FMT_YUV420P;
			colorspace = X264_CSP_I420;
			break;
	}

	xparams.i_width = outWidth;
	xparams.i_height = outHeight;
	xparams.i_fps_num = fps;
	xparams.i_fps_den = 1;
	xparams.b_intra_refresh = 0;
	xparams.i_keyint_max = gop;
	x264_param_apply_profile(&xparams, "baseline");
	return true;
}

void VideoEncoderX264::resizeEvent(Jzon::Node* params)
{
	if (!params) {
		return;
	}

	if (!params->Has("width") || !params->Has("height")) {
		return;
	}

	int width = params->Get("width").ToInt();
	int height = params->Get("height").ToInt();

	if (width)
		outWidth = width;

	if (height)
		outHeight = height;	

	sws_freeContext(swsCtx);
	swsCtx = sws_getContext(inWidth, inHeight, inPixel, outWidth, outHeight, outPixel, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	xparams.i_width = outWidth;
	xparams.i_height = outHeight;
	x264_param_apply_profile(&xparams, "baseline");
	encoder = x264_encoder_open(&xparams);
	x264_picture_alloc(&picIn, colorspace, outWidth, outHeight);
}

void VideoEncoderX264::changeBitrateEvent(Jzon::Node* params)
{
	if (!params) {
		return;
	}

	if (!params->Has("bitrate")) {
		return;
	}

	bitrate = params->Get("bitrate").ToInt();
	xparams.rc.i_bitrate = bitrate;
	x264_param_apply_profile(&xparams, "baseline");
}

void VideoEncoderX264::changeFramerateEvent(Jzon::Node* params)
{
	if (!params) {
		return;
	}

	if (!params->Has("framerate")) {
		return;
	}

	fps = params->Get("framerate").ToInt();
	xparams.i_fps_num = fps;
	x264_param_apply_profile(&xparams, "baseline");
}

void VideoEncoderX264::changeGOPEvent(Jzon::Node* params)
{	
	if (!params) {
		return;
	}

	if (!params->Has("gop")) {
		return;
	}

	gop = params->Get("gop").ToInt();
	xparams.i_keyint_max = gop;
	x264_param_apply_profile(&xparams, "baseline");
}

void VideoEncoderX264::forceIntraEvent(Jzon::Node* params)
{
	forceIntra = true;
}

void VideoEncoderX264::initializeEventMap()
{
	eventMap["resize"] = std::bind(&VideoEncoderX264::resizeEvent, this, std::placeholders::_1);
	eventMap["changeBitrate"] = std::bind(&VideoEncoderX264::changeBitrateEvent, this, std::placeholders::_1);
	eventMap["chengeFramerate"] = std::bind(&VideoEncoderX264::changeFramerateEvent, this, std::placeholders::_1);
	eventMap["changeGOP"] = std::bind(&VideoEncoderX264::changeGOPEvent, this, std::placeholders::_1);
	eventMap["forceIntra"] = std::bind(&VideoEncoderX264::forceIntraEvent, this, std::placeholders::_1);
}
