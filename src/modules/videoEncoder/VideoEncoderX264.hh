#ifndef _VIDEO_ENCODER_X264_HH
#define _VIDEO_ENCODER_X264_HH

#include <stdint.h>
#include "../../X264VideoFrame.hh"
#include "../../ProcessorInterface.hh"

extern "C" {
#include <x264.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


class VideoEncoderX264: public ProcessorInterface {
	public:
		VideoEncoderX264();
		~VideoEncoderX264();
		void encodeHeadersFrame(Frame *decodedFrame, Frame *encodedFrame);
		void encodeFrame(bool forceIntra, Frame *decodedFrame, Frame *encodedFrame);
		bool config(x264_param_t params, int inFps);

	protected:

		x264_picture_t picIn;
		x264_picture_t picOut;
		int fps;
		int pts;
		x264_param_t xparams;
		x264_t* encoder;
		struct SwsContext* swsCtx;
		
	private:
		//Extra functions
		bool open(int outWidth, int outHeight, AVPixelFormat outPixel);
		bool setParameters(x264_param_t params, int inW, int inH, int outW, int outH, int inFps, AVPixelFormat inPixelFormat, AVPixelFormat outPixelFormat);
		bool encodeHeaders(x264_nal_t **ppNal, int *piNal);
		int encode(bool forceIntra, uint8_t** pixels, x264_nal_t **ppNal, int *piNal);
		bool close();
		void print();//Debugging purpose
};

//#endif
