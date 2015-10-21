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
}

#include "../../VideoFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"
#include "../../StreamInfo.hh"


class VideoDecoderLibav : public OneToOneFilter {

public:
    VideoDecoderLibav();
    ~VideoDecoderLibav();
        
private:
    void initializeEventMap();
    FrameQueue* allocQueue(ConnectionData cData);
    bool doProcessFrame(Frame *org, Frame *dst);
    bool toBuffer(VideoFrame *decodedFrame, VideoFrame *codedFrame);
    bool reconfigure(VCodecType codec);
    bool inputConfig();
    void doGetState(Jzon::Object &filterNode);

    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    
    //NOTE: There is no need of specific writer configuration
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};
    
    AVCodec             *codec;
    AVCodecContext      *codecCtx;
    AVFrame             *frame, *frameCopy;
    AVPacket            pkt;
    AVCodecID           libavCodecId;

    

    StreamInfo *outputStreamInfo;

    struct InputStreamInfo {
            unsigned    inputWidth;
            unsigned    inputHeight;
            VCodecType  fCodec;
        };

    InputStreamInfo          psi;   
};

#endif
