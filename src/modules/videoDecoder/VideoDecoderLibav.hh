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

#ifndef _FRAME_HH
#include "../../Frame.hh"
#endif

enum CodecType {H264, VP8, MJPEG, RAW};

enum PixType {RGB24, RGB32, YUYV422};

class VideoDecoderLibav {

    public:
        VideoDecoderLibav();
        bool decodeFrame(Frame *codedFrame, Frame *decodedFrame);
        bool configDecoder(CodecType cType, PixType pTyp);
        
    private:
        bool toBuffer(Frame *decodedFrame);
        
        AVCodec             *codec;
        AVCodecContext      *codecCtx;
        struct SwsContext   *imgConvertCtx;
        AVFrame             *frame, *outFrame;
        AVPacket            pkt;
        AVPixelFormat       pix_fmt;
        AVCodecID           codec_id;
        CodecType           fCodec;
        PixType             fPixType;
};

#endif
