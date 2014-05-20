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
 */

#ifndef _X264_VIDEO_FRAME_HH
#define _X264_VIDEO_FRAME_HH

#include "VideoFrame.hh"

extern "C" {
	#include <x264.h>
}

class X264VideoFrame : public VideoFrame {

	public:
		void setNals(x264_nal_t **nals, int size);
		x264_nal_t** getNals() {return ppNals;};
		int getSize() {return sizeNals;};
		
	protected:
		x264_nal_t **ppNals;
		int sizeNals;
};

#endif
