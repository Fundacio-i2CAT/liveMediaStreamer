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

void VideoFrame::setSize(unsigned int width, unsigned int height)
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

InterleavedVideoFrame* InterleavedVideoFrame::createNew(unsigned int maxLength)
{
    return new InterleavedVideoFrame(maxLength);
}

InterleavedVideoFrame* InterleavedVideoFrame::createNew(unsigned int width, unsigned height, PixType pixelFormat)
{
    return new InterleavedVideoFrame(width, height, pixelFormat);
}

InterleavedVideoFrame::InterleavedVideoFrame(unsigned int maxLength)
{
    width = 0;
    height = 0;
    bufferLen = 0;
    bufferMaxLen = maxLength;
    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedVideoFrame::InterleavedVideoFrame(unsigned int width, unsigned height, PixType pixelFormat)
{
    this->width = width;
    this->height = height;
    this->pixelFormat = pixelFormat;

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
    bufferLen = 0;
    bufferMaxLen = width * height * bytesPerPixel;
    frameBuff = new unsigned char [bufferMaxLen]();
}
