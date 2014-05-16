#ifndef _VIDEO_ENCODER_X264_HH
#define _VIDEO_ENCODER_X264_HH

#include <stdint.h>

extern "C" {
#include <x264.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


class VideoEncoderX264 {
	public:
		VideoEncoderX264();
		~VideoEncoderX264();
		bool setParameters(x264_param_t params, int inW, int inH, int outW, int outH, int inFps, AVPixelFormat inPixelFormat, AVPixelFormat outPixelFormat);
		bool open();
		bool encodeHeaders(x264_nal_t **ppNal, int *piNal);
		int encode(bool forceIntra, uint8_t** pixels, x264_nal_t **ppNal, int *piNal);
		bool close();
		void print();//Debugging purpose

	private:
		int inWidth;
		int inHeight;
		int outWidth;
		int outHeight;
		x264_picture_t picIn;
		x264_picture_t picOut;
		int fps;
		int pts;
	  	AVPixelFormat inPixel;
	  	AVPixelFormat outPixel;
		x264_param_t xparams;
		x264_t* encoder;
		struct SwsContext* swsCtx;
		
		//Extra functions
		bool params();
};

//#endif
