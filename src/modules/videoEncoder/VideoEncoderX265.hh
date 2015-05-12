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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 *			  Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _VIDEO_ENCODER_X265_HH
#define _VIDEO_ENCODER_X265_HH

#include "VideoEncoderX264or5.hh"
#include <stdint.h>
#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../X265VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../Types.hh"

extern "C" {
#include <x265.h>
}

class VideoEncoderX265 : public VideoEncoderX264or5 {

public:
    VideoEncoderX265(FilterRole fRole = MASTER, bool sharedFrames = true);
	~VideoEncoderX265();
	FrameQueue* allocQueue(int wId);

private:
	void initializeEventMap();

    x265_picture    *picIn;
    x265_picture    *picOut;
	x265_param      *xparams;
    x265_encoder*   encoder;

    int64_t         pts;

    bool fillPicturePlanes(unsigned char** data, int* linesize);
    bool encodeFrame(VideoFrame* codedFrame);
    bool reconfigure(VideoFrame *orgFrame, VideoFrame* dstFrame);
    bool encodeHeadersFrame(X265VideoFrame* x265Frame);
};

#endif
