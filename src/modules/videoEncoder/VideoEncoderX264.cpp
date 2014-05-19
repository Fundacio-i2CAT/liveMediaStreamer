#include "VideoEncoderX264.hh"

VideoEncoderX264::VideoEncoderX264(){
	inWidth = 0;
	inHeight = 0;
	outWidth = 0;
	outHeight = 0;
	fps = 24;
	pts = 0;
	inPixel = AV_PIX_FMT_NONE;
	outPixel = AV_PIX_FMT_NONE;
	swsCtx = NULL;
	encoder = NULL;
	x264_param_default_preset(&xparams, "ultrafast", "zerolatency");
	xparams.i_threads = 1;
	xparams.i_width = outWidth;
	xparams.i_height = outHeight;
	xparams.i_fps_num = fps;
	xparams.i_fps_den = 1;
	xparams.b_intra_refresh = 0;
}

VideoEncoderX264::~VideoEncoderX264(){
	//delete encoder;
}

bool VideoEncoderX264::setParameters(x264_param_t params, int inW, int inH, int outW, int outH, int inFps, AVPixelFormat inPixelFormat, AVPixelFormat outPixelFormat) {
	bool ret = true;
	inWidth = inW;
	inHeight = inH;
	outWidth = outW;
	outHeight = outH;
	fps = inFps;
	inPixel = inPixelFormat;
	outPixel = outPixelFormat;
	xparams = params;
	x264_param_apply_profile(&xparams, "baseline");
	swsCtx = sws_getContext(inWidth, inHeight, inPixel, outWidth, outHeight, outPixel, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	return ret;
}

bool VideoEncoderX264::open() {
	if(!params()) {
		printf("The parameters have not been configured\n");
    	return false;
	}

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

int VideoEncoderX264::encode(bool forceIntra, uint8_t** pixels, x264_nal_t **ppNal, int *piNal) {
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

bool VideoEncoderX264::params() {
	bool ret = true;
	if(!inWidth) {
		printf("Error: without in_width\n");
		ret= false;
	}

	if(!inHeight) {
		printf("Error: without in_height\n");
		ret= false;
	}

	if(!outWidth) {
		printf("Error: without out_width\n");
		ret= false;
	}

	if(!outHeight) {
		printf("Error: without out_height\n");
		return false;
	}

	if(inPixel == AV_PIX_FMT_NONE) {
		printf("Error: without in_pixel_format\n");
		ret= false;
	}
	if(outPixel == AV_PIX_FMT_NONE) {
		printf("Error: without out_pixel_format\n");
		ret= false;
	}
	return ret;
}

void VideoEncoderX264::print() {
	printf("in_width %d\n", inWidth);
	printf("in_height %d\n", inHeight);
	printf("out_width %d\n", outWidth);
	printf("out_height %d\n", outHeight);
	printf("fps %d\n", fps);
	printf("pts %d\n", pts);
}
