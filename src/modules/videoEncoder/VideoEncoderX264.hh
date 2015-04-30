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
 *            Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _VIDEO_ENCODER_X264_HH
#define _VIDEO_ENCODER_X264_HH

#include "VideoEncoderX264or5.hh"
#include <stdint.h>
#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../X264VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../X264VideoCircularBuffer.hh"
#include "../../Types.hh"

extern "C" {
#include <x264.h>
}

#define MAX_PLANES_PER_PICTURE 4

class VideoEncoderX264 : public VideoEncoderX264or5 {

public:
	VideoEncoderX264(FilterRole fRole = MASTER, bool sharedFrames = true);
	~VideoEncoderX264();
	FrameQueue* allocQueue(int wId);

private:
	void initializeEventMap();

    x264_picture_t picIn;
	x264_picture_t picOut;
    x264_param_t xparams;
    x264_t* encoder;
    x264_nal_t *ppNal;

    PixType inPixFmt;
    int64_t pts;

    bool fillPicturePlanes(unsigned char** data, int* linesize);
    bool encodeFrame(VideoFrame* codedFrame);
    bool reconfigure(VideoFrame *orgFrame, VideoFrame* dstFrame);
    bool encodeHeadersFrame(X264VideoFrame* x264Frame);
    bool fill_x264_picture(VideoFrame* videoFrame);
};

#endif
