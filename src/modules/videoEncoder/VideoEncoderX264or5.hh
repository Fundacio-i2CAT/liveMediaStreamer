/*
 *  VideoEncoderX264or5 - Base class for VideoEncoderX264 and VideoEncoderX265
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>
 */

#ifndef _VIDEO_ENCODER_X264_OR_5_HH
#define _VIDEO_ENCODER_X264_OR_5_HH

#include <stdint.h>
#include <chrono>
#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../Types.hh"

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/*! Base class for VideoEncoderX264 and VideoEncoderX265. It implements common methods, basically configure and doProcessFrame */

class VideoEncoderX264or5 : public OneToOneFilter {
    
public:
    /**
    * Class constructor
    * @param fRole Filter role (NETWORK, MASTER, SLAVE)
    * @param sharedFrames If true and fRole is MASTER, it will share input frames with its slave filters
    */
    VideoEncoderX264or5(FilterRole fRole, bool sharedFrames);

    /**
    * Class destructor
    */
    virtual ~VideoEncoderX264or5();

    bool configure(int bitrate_, int fps_, int gop_, int lookahead_, int threads_, bool annexB_, std::string preset_);
protected:
    AVPixelFormat libavInPixFmt;
    AVFrame *midFrame;
    
    PixType inPixFmt;
    bool annexB;
    bool forceIntra;
    int fps;
    int bitrate;
    int gop;
    int threads;
    int lookahead;
    bool needsConfig;
    std::string preset;
    
    bool doProcessFrame(Frame *org, Frame *dst);
    void initializeEventMap();      
    virtual bool fillPicturePlanes(unsigned char** data, int* linesize) = 0;
    virtual bool encodeFrame(VideoFrame* codedFrame) = 0;
    virtual bool reconfigure(VideoFrame* orgFrame, VideoFrame* dstFrame) = 0;
    void setIntra(){forceIntra = true;};
    bool fill_x264or5_picture(VideoFrame* videoFrame);
    void forceIntraEvent(Jzon::Node* params);
    void configEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void doGetState(Jzon::Object &filterNode);
};

#endif