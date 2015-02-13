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

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include "../../VideoFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"


class VideoDecoderLibav : public OneToOneFilter {

    public:
        VideoDecoderLibav();
        ~VideoDecoderLibav();
        bool doProcessFrame(Frame *org, Frame *dst);
        FrameQueue* allocQueue(int wId);
        bool configure(int width, int height, PixType pixelFormat);
        
    private:
        void initializeEventMap();
        bool toBuffer(VideoFrame *decodedFrame, VideoFrame *codedFrame);
        bool reconfigure(VCodecType codec);
        bool inputConfig();
        void doGetState(Jzon::Object &filterNode);
        
        AVCodec             *codec;
        AVCodecContext      *codecCtx;
        AVFrame             *frame, *frameCopy;
        AVPacket            pkt;
        AVCodecID           libavCodecId;

        VCodecType          fCodec;
};

#endif
