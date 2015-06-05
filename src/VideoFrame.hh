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
#include "Utils.hh"

#define MAX_COPIED_SLICES 8
#define MAX_SLICES 16

class VideoFrame : public Frame {

public:
    VideoFrame(VCodecType codec_);
    VideoFrame(VCodecType codec_, int width_, int height_, PixType pixFormat);
    virtual ~VideoFrame();

    void setSize(int width, int height);
    void setPixelFormat(PixType pixelFormat);
    
    VCodecType getCodec() {return codec;};
    int getWidth() {return width;};
    int getHeight() {return height;};
    PixType getPixelFormat() {return pixelFormat;};

protected:
    VCodecType codec;
    int width, height;
    PixType pixelFormat;
};

class InterleavedVideoFrame : public VideoFrame {
    
public:
    static InterleavedVideoFrame* createNew(VCodecType codec, unsigned int maxLength);
    static InterleavedVideoFrame* createNew(VCodecType codec, int width, int height, PixType pixelFormat);
    ~InterleavedVideoFrame();

    unsigned char **getPlanarDataBuf() {return NULL;};
    unsigned char* getDataBuf() {return frameBuff;};
    unsigned int getLength() {return bufferLen;};
    unsigned int getMaxLength() {return bufferMaxLen;};
    void setLength(unsigned int length) {bufferLen = length;};
    bool isPlanar() {return false;};

protected:
    InterleavedVideoFrame(VCodecType codec, unsigned int maxLength);
    InterleavedVideoFrame(VCodecType codec, int width, int height, PixType pixelFormat);

private:
    unsigned char *frameBuff;
    unsigned int bufferLen;
    unsigned int bufferMaxLen;
};

class Slice {

public:
    Slice();
    unsigned char* getData() {return data;};
    unsigned getDataSize() {return dataSize;};
    void setData(unsigned char *p) {data = p;};
    void setDataSize(unsigned s) {dataSize = s;};
    void allocData(unsigned size);
    void releaseData();
    void copyData(unsigned char *p, unsigned s);

private:
    unsigned char *data;
    unsigned dataSize;
};

class SlicedVideoFrame : public VideoFrame {

public:
    static SlicedVideoFrame* createNew(VCodecType codec, unsigned copiedSlicesMaxSize);
    virtual ~SlicedVideoFrame();
    void clear();

    Slice* getSlices() {return pointedSlices;};
    Slice* getCopiedSlices() {return copiedSlices;};
    bool setSlice(unsigned char *data, unsigned size);
    bool copySlice(unsigned char *data, unsigned size);

    int getSliceNum() {return pointedSliceNum;};
    int getCopiedSliceNum() {return copiedSliceNum;};

    unsigned char *getDataBuf() {return NULL;};
    unsigned char **getPlanarDataBuf() {return NULL;};
    unsigned int getLength() {return 0;};
    unsigned int getMaxLength() {return 0;};
    void setLength(unsigned int length) {};
    bool isPlanar() {return false;};

private:
    SlicedVideoFrame(VCodecType codec, unsigned copiedSlicesMaxSize);

    Slice pointedSlices[MAX_SLICES];
    Slice copiedSlices[MAX_COPIED_SLICES];

    int pointedSliceNum;
    int copiedSliceNum;
    unsigned copiedSlicesMaxSize;
};




#endif
