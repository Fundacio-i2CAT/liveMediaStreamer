/*
 *  VideoResampler.hh - A libav-based video resampler
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

#ifndef _VIDEO_RESAMPLER_HH
#define _VIDEO_RESAMPLER_HH

extern "C" {
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

#include "../../VideoFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"

class VideoResampler : public OneToOneFilter {

    public:
        VideoResampler(FilterRole fRole_ = MASTER, bool sharedFrames = true);
        ~VideoResampler();
        bool doProcessFrame(Frame *org, Frame *dst);
        FrameQueue* allocQueue(int wId);
        bool configure(int width, int height, int period, PixType pixelFormat);
        
    private:
        void initializeEventMap();
        bool configEvent(Jzon::Node* params);
        void doGetState(Jzon::Object &filterNode);
        bool reconfigure(VideoFrame* orgFrame);
        bool setAVFrame(AVFrame *aFrame, VideoFrame* vFrame, AVPixelFormat format);
        
        struct SwsContext   *imgConvertCtx;
        AVFrame             *inFrame, *outFrame;
        AVPixelFormat       libavInPixFmt, libavOutPixFmt;

        int                 outputWidth;
        int                 outputHeight;
        int                 discartCount;
        int                 discartPeriod;
        PixType             inPixFmt, outPixFmt;
        bool                needsConfig;
};

#endif

