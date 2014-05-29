/*
 *  VideoDecoderLibav.hh - A libav-based video decoder
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors: David Cassany <david.cassany@i2cat.net>  
 *           Marc Palau <marc.palau@i2cat.net>
 */

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

#include "../../FrameQueue.hh"
#include "../../Filter.hh"




class VideoDecoderLibav : public OneToOneFilter {

    public:
        VideoDecoderLibav();
        bool doProcessFrame(Frame *org, Frame *dst);
        FrameQueue* allocQueue(int wId);
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
        VCodecType          fCodec;
        PixType             fPixType;
};

#endif
