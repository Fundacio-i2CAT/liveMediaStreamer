/*
 *  X264VideoFrame - X264 Video frame structure
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors: Martin German <martin.german@i2cat.net>
 *           David Cassany <david.cassany@i2cat.net>
 */

#ifndef _X264_VIDEO_FRAME_HH
#define _X264_VIDEO_FRAME_HH

#include "VideoFrame.hh"

extern "C" {
	#include <x264.h>
}

#define MAX_HEADER_NALS 8
#define MAX_NALS_PER_FRAME 16
#define MAX_HEADER_NAL_SIZE 4096

class X264VideoFrame : public InterleavedVideoFrame {

public:
	static X264VideoFrame* createNew(unsigned int width, unsigned int height, PixType pixelFormat);
	~X264VideoFrame();

    x264_nal_t** getNals() {return nals;};
	x264_nal_t** getHdrNals() {return hdrNals;};
    void clearNalNum();


    void setNalNum(int num) {nalsNum = num;};
    int getNalsNum() {return nalsNum;};

    void setHdrNalNum(int num) {hdrNalsNum = num;};
    int getHdrNalsNum() {return hdrNalsNum;};
        
private:
	X264VideoFrame(unsigned int width, unsigned height, PixType pixelFormat);
        
    x264_nal_t **nals;
    x264_nal_t **hdrNals;
        
    int nalsNum;
    int hdrNalsNum;
};

#endif
