#ifndef _VIDEO_DECODER_LIBAV_HH
#define _VIDEO_DECODER_LIBAV_HH

//#ifndef __STDC_CONSTANT_MACROS
//#define __STDC_CONSTANT_MACROS
//#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#ifndef _VIDEO_FRAME_HH
#include "../../VideoFrame.hh"
#endif




class VideoDecoderLibav {

    public:
        VideoDecoderLibav();
        bool decodeFrame(Frame *codedFrame, Frame *decodedFrame);
        bool configDecoder(VCodecType cType, PixType pTyp);
        
    private:
        bool toBuffer(VideoFrame *decodedFrame);
        
        AVCodec             *codec;
        AVCodecContext      *codecCtx;
        struct SwsContext   *imgConvertCtx;
        AVFrame             *frame, *outFrame;
        AVPacket            pkt;
        AVPixelFormat       pix_fmt;
        AVCodecID           codec_id;
        VCodecType           fCodec;
        PixType             fPixType;
};

#endif
