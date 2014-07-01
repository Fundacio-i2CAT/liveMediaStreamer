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

#ifndef _VIDEO_FRAME_HH
#define _VIDEO_FRAME_HH

#include "Frame.hh"
#include "Types.hh"

class VideoFrame : public Frame {
    
    public:
        void setSize(unsigned int width, unsigned int height);
		void setPixelFormat(PixType pixelFormat);
        VCodecType getCodec() {return codec;};
        unsigned int getWidth() {return width;};
        unsigned int getHeight() {return height;};
		PixType getPixelFormat() {return pixelFormat;};
              
    protected:
        unsigned int width, height;
        PixType pixelFormat;
        VCodecType codec;
};

class InterleavedVideoFrame : public VideoFrame {
    public:
        static InterleavedVideoFrame* createNew(VCodecType codec, unsigned int maxLength);
        static InterleavedVideoFrame* createNew(VCodecType codec, unsigned int width, unsigned height, PixType pixelFormat);
        unsigned char* getDataBuf() {return frameBuff;};
        unsigned int getLength() {return bufferLen;};
        unsigned int getMaxLength() {return bufferMaxLen;};
        void setLength(unsigned int length) {bufferLen = length;};
        bool isPlanar() {return false;};
              
    private:
        InterleavedVideoFrame(VCodecType codec, unsigned int maxLength);
        InterleavedVideoFrame(VCodecType codec, unsigned int width, unsigned height, PixType pixelFormat);
        unsigned char *frameBuff;
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

#endif
