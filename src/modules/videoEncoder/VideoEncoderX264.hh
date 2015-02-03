/*
 *  VideoEncoderX264 - Video Encoder X264
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
 *  Authors:  Martin German <martin.german@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>
 */

#ifndef _VIDEO_ENCODER_X264_HH
#define _VIDEO_ENCODER_X264_HH

#include <stdint.h>
#include <chrono>
#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../X264VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../X264VideoCircularBuffer.hh"
#include "../../Types.hh"

extern "C" {
#include <x264.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define DEFAULT_ENCODER_THREADS 4
#define DEFAULT_GOP 2000 //ms
#define DEFAULT_BITRATE 2000

class VideoEncoderX264: public OneToOneFilter {
	public:
		VideoEncoderX264(int framerate = VIDEO_DEFAULT_FRAMERATE);
		~VideoEncoderX264();
		bool doProcessFrame(Frame *org, Frame *dst);
        bool configure(int gop_ = DEFAULT_GOP,
                       int bitrate_ = DEFAULT_BITRATE, 
                       int threads_ = DEFAULT_ENCODER_THREADS, int fps_ = VIDEO_DEFAULT_FRAMERATE, bool annexB_ = false);
		void setIntra(){forceIntra = true;};
		FrameQueue* allocQueue(int wId);

    private:
		void initializeEventMap();		
        
		x264_picture_t picIn;
		x264_picture_t picOut;
        x264_nal_t *ppNal;
        x264_param_t xparams;
        x264_t* encoder;
        
        AVPixelFormat libavInPixFmt;
        AVFrame *midFrame;
        
        PixType inPixFmt;
        bool annexB;
		bool forceIntra;
		bool firstTime;
		bool needsConfig;
		int fps;
		int pts;
		int bitrate;
		int colorspace;
		int gop; //ms
        int threads;
        
        bool reconfigure(VideoFrame *orgFrame, X264VideoFrame* x264Frame);
        bool encodeHeadersFrame(X264VideoFrame* x264Frame);
        bool fill_x264_picture(VideoFrame* videoFrame);
		void forceIntraEvent(Jzon::Node* params);
        void configEvent(Jzon::Node* params, Jzon::Object &outputNode);
		void doGetState(Jzon::Object &filterNode);
};

#endif
