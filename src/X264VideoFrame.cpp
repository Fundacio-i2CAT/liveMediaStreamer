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

#include "X264VideoFrame.hh"
#include <string>
#include "Utils.hh"
#include <cstring>

X264VideoFrame* X264VideoFrame::createNew(unsigned maxLength)
{
    return new X264VideoFrame(maxLength);
}

X264VideoFrame::X264VideoFrame(unsigned maxLength) :
InterleavedVideoFrame(H264, maxLength), nalsNum(0), hdrNalsNum(0)
{   
    hdrNals = (x264_nal_t**)malloc(sizeof(x264_nal_t*) * MAX_HEADER_NALS); 
    for (int i = 0; i < MAX_HEADER_NALS; i++) {
        hdrNals[i] = (x264_nal_t*)malloc(sizeof(x264_nal_t));
    }

    nals = (x264_nal_t**)malloc(sizeof(x264_nal_t*) * MAX_NALS_PER_FRAME); 
    for (int i = 0; i < MAX_NALS_PER_FRAME; i++) {
        nals[i] = (x264_nal_t*)malloc(sizeof(x264_nal_t));
    }
}

X264VideoFrame::~X264VideoFrame()
{
    delete[] hdrNals;
    delete[] nals;
}

void X264VideoFrame::clearNalNum()
{
    nalsNum = 0; 
    hdrNalsNum = 0;
}
