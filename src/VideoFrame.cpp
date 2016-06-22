/*
 *  VideoFrame - Video frame structure
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
 *  Authors: David Cassany <david.cassany@i2cat.net> 
 *           Marc Palau <marc.palau@i2cat.net>
 */

 #include "VideoFrame.hh"
 #include <string.h>

VideoFrame::VideoFrame(VCodecType codec_) : 
Frame(), codec(codec_), width(0), height(0), pixelFormat(P_NONE)
{

}

VideoFrame::VideoFrame(VCodecType codec_, int width_, int height_, PixType pixFormat)
: Frame(), codec(codec_), width(width_), height(height_), pixelFormat(pixFormat)
{

}

VideoFrame::~VideoFrame()
{

}

void VideoFrame::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
}

void VideoFrame::setPixelFormat(PixType pixelFormat)
{
    this->pixelFormat = pixelFormat;
}

//////////////////////////////////////////////////
//INTERLEAVED VIDEO FRAME METHODS IMPLEMENTATION//
//////////////////////////////////////////////////

InterleavedVideoFrame* InterleavedVideoFrame::createNew(VCodecType codec, unsigned int maxLength)
{
    return new InterleavedVideoFrame(codec, maxLength);
}

InterleavedVideoFrame* InterleavedVideoFrame::createNew(VCodecType codec, int width, int height, PixType pixelFormat)
{
    return new InterleavedVideoFrame(codec, width, height, pixelFormat);
}

InterleavedVideoFrame::InterleavedVideoFrame(VCodecType codec, unsigned int maxLength)
: VideoFrame(codec), bufferLen(0)
{
    bufferMaxLen = maxLength;
    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedVideoFrame::InterleavedVideoFrame(VCodecType codec, int width, int height, PixType pixelFormat)
: VideoFrame(codec, width, height, pixelFormat), bufferLen(0)
{
    int bytesPerPixel;

    switch (pixelFormat) {
        case RGB24:
            bytesPerPixel = 3;
            break;
        case RGB32:
            bytesPerPixel = 4;
            break;
        case YUYV422:
            bytesPerPixel = 2;
            break;
        default:
            bytesPerPixel = DEFAULT_BYTES_PER_PIXEL;
            break;
    }

    bufferMaxLen = width * height * bytesPerPixel;
    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedVideoFrame::~InterleavedVideoFrame()
{
    delete[] frameBuff;
}

/////////////////////////
// X264or5 VIDEO FRAME //
/////////////////////////

SlicedVideoFrame* SlicedVideoFrame::createNew(VCodecType codec) 
{
    return new SlicedVideoFrame(codec);
}

SlicedVideoFrame::SlicedVideoFrame(VCodecType codec) :
VideoFrame(codec), pointedSliceNum(0) 
{

}

SlicedVideoFrame::~SlicedVideoFrame()
{

}

void SlicedVideoFrame::clear()
{
    pointedSliceNum = 0; 
}

bool SlicedVideoFrame::setSlice(unsigned char *data, unsigned size)
{
    if (pointedSliceNum >= MAX_SLICES) {
        return false;
    }

    pointedSlices[pointedSliceNum].setData(data);
    pointedSlices[pointedSliceNum].setDataSize(size);

    pointedSliceNum++;
    return true;
}


Slice::Slice() : data(NULL), dataSize(0)
{

}
