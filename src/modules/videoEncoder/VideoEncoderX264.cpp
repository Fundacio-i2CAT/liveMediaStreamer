#include "VideoEncoderX264.hh"

VideoEncoderX264::VideoEncoderX264(): OneToOneFilter(){
	fps = 24;
	pts = 0;
	swsCtx = NULL;
	encoder = NULL;
	x264_param_default_preset(&xparams, "ultrafast", "zerolatency");
	xparams.i_threads = 1;
	xparams.i_width = 1280;
	xparams.i_height = 720;
	xparams.i_fps_num = fps;
	xparams.i_fps_den = 1;
	xparams.b_intra_refresh = 0;
	x264_param_apply_profile(&xparams, "baseline");
}

VideoEncoderX264::~VideoEncoderX264(){
	//delete encoder;
}

bool doProcessFrame(Frame *org, Frame *dst) {
	InterleavedVideoFrame* interleavedFrame = dynamic_cast<InterleavedVideoFrame*> org;
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> dst;
	int inWidth, inHeight, outWidth, outHeight, inFps, frameLength, pixelSize; 
	x264_nal_t **ppNal;
	int *piNal;
	AVPixelFormat inPixel, outPixel;
	PixType inPixelType, outPixelType;
	inWidth = interleavedFrame->getWidth();
	inHeight = interleavedFrame->getHeight();
	outWidth = x264Frame->getWidth();
	outHeight = x264Frame->getHeight();
	inPixelType = interleavedFrame->getPixelFormat();
	outPixelType = x264Frame->getPixelFormat();

	switch (inPixelType) {
		P_NONE:
			inPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			inPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			inPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			inPixel = PIX_FMT_YUV420P;
			break;
	}

	switch (outPixelType) {
		P_NONE:
			outPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			outPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			outPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			outPixel = PIX_FMT_YUV420P;
			break;
	}

	pixelSize = inWidth*3;

	picIn.i_pts = pts;

	if (forceIntra) {
		picIn.i_type = X264_TYPE_I;
		forceIntra = false;
	}
	else
		picIn.i_type = X264_TYPE_AUTO;
	
	sws_scale(swsCtx, pixels, &pixelSize, 0, inHeight, picIn.img.plane, picIn.img.i_stride);

	frameLength = x264_encoder_encode(encoder, ppNal, piNal, &picIn, &picOut);
	if (frameLength < 1) {
		std::cerr << "Error encoding video frame" << std::endl;
		return false;
	}

	x264Frame->setNals(ppNal, (*piNal), frameLength);

	pts++;
	return true;
}


void VideoEncoderX264::encodeHeadersFrame(Frame *decodedFrame, Frame *encodedFrame) {
	InterleavedVideoFrame* interleavedFrame = dynamic_cast<InterleavedVideoFrame*> decodedFrame;
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> encodedFrame;
	int inWidth, inHeight, outWidth, outHeight, inFps, encodeSize;
	x264_nal_t **ppNal;
	int *piNal;
	AVPixelFormat inPixel, outPixel;
	PixType inPixelType, outPixelType;
	inWidth = interleavedFrame->getWidth();
	inHeight = interleavedFrame->getHeight();
	outWidth = x264Frame->getWidth();
	outHeight = x264Frame->getHeight();
	inPixelType = interleavedFrame->getPixelFormat();
	outPixelType = x264Frame->getPixelFormat();

	switch (inPixelType) {
		P_NONE:
			inPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			inPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			inPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			inPixel = PIX_FMT_YUV420P;
			break;
	}

	switch (outPixelType) {
		P_NONE:
			outPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			outPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			outPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			outPixel = PIX_FMT_YUV420P;
			break;
	}

	swsCtx = sws_getContext(inWidth, inHeight, inPixel, outWidth, outHeight, outPixel, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	xparams.i_width = outWidth;
	xparams.i_height = outHeight;
	x264_param_apply_profile(&xparams, "baseline");
	encoder = x264_encoder_open(&xparams);
	x264_picture_alloc(&picIn, X264_CSP_I420, outWidth, outHeight);
	
	encodeSize = x264_encoder_headers(encoder, ppNal, piNal);
	if (encodeh < 0) {
		printf("Error: encoder headers\n");
	}
	x264Frame->setNals(ppNal, (*piNal), encodeSize);
}

void VideoEncoderX264::encodeFrame(Frame *decodedFrame, Frame *encodedFrame) {
	InterleavedVideoFrame* interleavedFrame = dynamic_cast<InterleavedVideoFrame*> decodedFrame;
	X264VideoFrame* x264Frame = dynamic_cast<X264VideoFrame*> encodedFrame;
	int inWidth, inHeight, outWidth, outHeight, inFps, frameLength, pixelSize; 
	x264_nal_t **ppNal;
	int *piNal;
	AVPixelFormat inPixel, outPixel;
	PixType inPixelType, outPixelType;
	inWidth = interleavedFrame->getWidth();
	inHeight = interleavedFrame->getHeight();
	outWidth = x264Frame->getWidth();
	outHeight = x264Frame->getHeight();
	inPixelType = interleavedFrame->getPixelFormat();
	outPixelType = x264Frame->getPixelFormat();

	switch (inPixelType) {
		P_NONE:
			inPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			inPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			inPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			inPixel = PIX_FMT_YUV420P;
			break;
	}

	switch (outPixelType) {
		P_NONE:
			outPixel = AV_PIX_FMT_NONE;
			break;
		RGB24:
			outPixel = PIX_FMT_RGB24;
			break;
		RGB32:
			outPixel = PIX_FMT_RGBA;
			break;
		YUYV422:
			outPixel = PIX_FMT_YUV420P;
			break;
	}

	pixelSize = inWidth*3;

	picIn.i_pts = pts;

	if (forceIntra) {
		picIn.i_type = X264_TYPE_I;
		forceIntra = false;
	}
	else
		picIn.i_type = X264_TYPE_AUTO;
	
	sws_scale(swsCtx, pixels, &pixelSize, 0, inHeight, picIn.img.plane, picIn.img.i_stride);

	frameLength = x264_encoder_encode(encoder, ppNal, piNal, &picIn, &picOut);
	if (frameLength < 1) {
		printf ("Error: x264_encoder_encode\n");
	}

	x264Frame->setNals(ppNal, (*piNal), frameLength);

	pts++;

}

FrameQueue* VideoEncoderX264::allocQueue(int wId) {
	return X264VideoCircularBuffer::createNew();
}

bool VideoEncoderX264::config(x264_param_t params, int inFps) {
	xparams = params;
	fps = inFps;
	x264_param_apply_profile(&xparams, "baseline");
	return true;
}

bool VideoEncoderX264::setParameters(x264_param_t params, int inWidth, int inHeight, int outWidth, int outHeight, int inFps, AVPixelFormat inPixel, AVPixelFormat outPixel) {
	bool ret = true;
	fps = inFps;
	xparams = params;
	x264_param_apply_profile(&xparams, "baseline");
	swsCtx = sws_getContext(inWidth, inHeight, inPixel, outWidth, outHeight, outPixel, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	return ret;
}

bool VideoEncoderX264::open(int outWidth, int outHeight, AVPixelFormat outPixel) {
	if (encoder) {
		printf("Encoder already opened\n");
		return false;
	}

	if((outPixel) != AV_PIX_FMT_YUV420P) {
		printf ("Output format must be AV_PIX_FMT_YUV420P\n");
		return false;
	}

	if (!swsCtx) {
		printf("Error: Cannot create the context\n");
		return false;
	}

	encoder = x264_encoder_open(&xparams);
	x264_picture_alloc(&picIn, X264_CSP_I420, outWidth, outHeight);
	
	if (!encoder) {
		printf("Error: open encoder\n");
		return false;
	}

	pts = 0;

	return true;
}

bool VideoEncoderX264::encodeHeaders(x264_nal_t **ppNal, int *piNal) {
	int encodeh;
	
	encodeh = x264_encoder_headers(encoder, ppNal, piNal);
	if (encodeh < 0) {
		printf("Error: encoder headers\n");
		return false;
	}
	
	return true;
}

int VideoEncoderX264::encode(bool forceIntra, uint8_t** pixels, x264_nal_t **ppNal, int *piNal, int inWidth) {
	int frameLength, pixelSize = inWidth*3;

	picIn.i_pts = pts;

	if (forceIntra) {//Doesn't work
		x264_encoder_intra_refresh(encoder);
	}
	
	sws_scale(swsCtx, pixels, &pixelSize, 0, inHeight, picIn.img.plane, picIn.img.i_stride);

	frameLength = x264_encoder_encode(encoder, ppNal, piNal, &picIn, &picOut);
	if (frameLength < 1) {
		printf ("Error: x264_encoder_encode\n");
		return false;
	}

	pts++;
	return frameLength;
}

bool VideoEncoderX264::close() {
	if(encoder)
		x264_picture_clean(&picIn);
	x264_encoder_close(encoder);
    encoder = NULL;
	return true;
}

void VideoEncoderX264::print() {
	printf("fps %d\n", fps);
	printf("pts %d\n", pts);
}
