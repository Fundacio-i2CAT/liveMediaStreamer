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

X264VideoFrame* X264VideoFrame::createNew(unsigned int width, unsigned height, PixType pixelFormat)
{
    return new X264VideoFrame(width, height, pixelFormat);
}

X264VideoFrame::X264VideoFrame(unsigned int width, unsigned height, PixType pixelFormat) :
    InterleavedVideoFrame(H264, width, height, pixelFormat)
{   
    this->hNalsNum = 0;
    this->headerLength = 0;
    
    for(int i = 0; i < MAX_HEADER_NALS; i++) {
        headerNals[i] = (unsigned char *) malloc(sizeof(unsigned char)*MAX_HEADER_NAL_SIZE);
    }
}

X264VideoFrame::~X264VideoFrame()
{
    clearNals();
    for(int i = 0; i < MAX_HEADER_NALS; i++){
    	delete[] headerNals[i];
    }
}

void X264VideoFrame::setNals(x264_nal_t **nals, int num, int frameSize)
{
    ppNals = nals;
    nalsNum = num;
    frameLength = frameSize;
}

void X264VideoFrame::setHeaderNals(x264_nal_t **nals, int num, int headerSize)
{
    hNalsNum = num;
    headerLength = headerSize;
    
    for(int i = 0; i < hNalsNum; i++){       
        memcpy(headerNals[i], (*nals)[i].p_payload, (*nals)[i].i_payload);
        hNalSize[i] = (*nals)[i].i_payload;
    }
}

void X264VideoFrame::clearNals()
{
    hNalsNum = 0; 
    headerLength = 0;
    nalsNum = 0;
    frameLength = 0;
}
