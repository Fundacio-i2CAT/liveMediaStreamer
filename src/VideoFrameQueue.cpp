/*
 *  VideoFrameQueue - A lock-free video frame circular queue
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#include "VideoFrameQueue.hh"
#include "InterleavedVideoFrame.hh"

VideoFrameQueue* VideoFrameQueue::createNew(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat)
{
    return new VideoFrameQueue(codec, delay, width, height, pixelFormat);
}

VideoFrameQueue::VideoFrameQueue(VCodecType codec, unsigned delay, unsigned width, unsigned height, PixType pixelFormat)
{
    this->codec = codec;
    this->delay = delay;
    this->width = width;
    this->height = height;
    this->pixelFormat = pixelFormat;

    config();
}

bool VideoFrameQueue::config()
{
    switch(codec) {
        case H264:
            max = DEFAULT_VIDEO_FRAMES;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedVideoFrame(LENGTH_H264);
            }
            break;
        case VP8:
            //TODO: implement this initialization
            break;
        case MJPEG:
            //TODO: implement this initialization
            break;
        case RAW:
            if (pixelFormat == P_NONE || width == 0 || height == 0) {
                //TODO: error message
                break;
            }
            max = DEFAULT_VIDEO_FRAMES;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedVideoFrame(width, height, pixelFormat);
            }
            break;
        default:
            //TODO: error message
            break;
    }
}

