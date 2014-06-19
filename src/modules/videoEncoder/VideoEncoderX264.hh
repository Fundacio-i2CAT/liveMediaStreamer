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
 */

#ifndef _VIDEO_ENCODER_X264_HH
#define _VIDEO_ENCODER_X264_HH

#include <stdint.h>
#include "../../VideoFrame.hh"
#include "../../X264VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../X264VideoCircularBuffer.hh"

extern "C" {
#include <x264.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


class VideoEncoderX264: public OneToOneFilter {
	public:
		VideoEncoderX264();
		~VideoEncoderX264();
		bool doProcessFrame(Frame *org, Frame *dst);
		bool configure(int width, int height, PixType pixelFormat);		
		void setIntra(){forceIntra = true;};
		FrameQueue* allocQueue(int wId);
		void initializeEventMap();
		

	private:
		x264_picture_t picIn;
		x264_picture_t picOut;
		AVPixelFormat inPixel;
		AVPixelFormat outPixel;
		bool forceIntra;
		bool firstTime;
		bool configureOut;
		int fps;
		int pts;
		int inWidth;
		int inHeight;
		int outWidth;
		int outHeight;
		int bitrate;
		int colorspace;
		int gop;
		x264_param_t xparams;
		x264_t* encoder;
		struct SwsContext* swsCtx;
		
		bool config(Frame *org, Frame *dst);
		void encodeHeadersFrame(Frame *decodedFrame, Frame *encodedFrame);
		void resizeEvent(Jzon::Node* params);
		void changeBitrateEvent(Jzon::Node* params);
		void changeFramerateEvent(Jzon::Node* params);
		void changeGOPEvent(Jzon::Node* params);
		void forceIntraEvent(Jzon::Node* params);
		void doGetState(Jzon::Object &filterNode);
};

#endif
