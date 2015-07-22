/*
 *  FrameMockUp - Frame class mockups
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
 *  Authors:    David Cassany <david.cassany@i2cat.net>
 *
 */

#ifndef _FRAME_MOCKUP_HH
#define _FRAME_MOCKUP_HH

#include "Frame.hh"
#include "VideoFrame.hh"

class FrameMock : public Frame {
public:
    ~FrameMock(){};
    virtual unsigned char *getDataBuf() {
        return buff;
    };

    static FrameMock* createNew(size_t seqNum) {
        FrameMock *frame = new FrameMock();
        frame->setSequenceNumber(seqNum);
        return frame;
    }

    virtual unsigned char **getPlanarDataBuf() {return NULL;};
    virtual unsigned int getLength() {return 4;};
    virtual unsigned int getMaxLength() {return 4;};
    virtual void setLength(unsigned int length) {};
    virtual bool isPlanar() {return false;};

protected:
    unsigned char buff[4];
};

class VideoFrameMock : public InterleavedVideoFrame
{
public:
    ~VideoFrameMock(){};
    virtual unsigned char *getDataBuf() {
        return buff;
    };

    static VideoFrameMock* createNew() {
        return new VideoFrameMock();
    };

    virtual unsigned char **getPlanarDataBuf() {return NULL;};
    virtual unsigned int getLength() {return 4;};
    virtual unsigned int getMaxLength() {return 4;};
    virtual void setLength(unsigned int length) {};
    virtual bool isPlanar() {return false;};

protected:
    unsigned char buff[4] = {1,2,1,2};

private:
    VideoFrameMock(): InterleavedVideoFrame(RAW, 1920, 1080, YUV420P) {};
};

#endif
