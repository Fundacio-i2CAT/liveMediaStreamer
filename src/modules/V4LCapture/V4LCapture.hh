/*
 *  V4LCapture.hh - Capture based on V4L2
 * 
 *  Copyright (C) 2016  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 */

#ifndef _V4L_CAPTURE_HH
#define _V4L_CAPTURE_HH

#include <string>
#include <chrono>
#include <linux/videodev2.h>

#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../StreamInfo.hh"
#include "../../Filter.hh"

struct buffer {
      void   *data;
      size_t  size;
};

enum DeviceStatus {OPEN, INIT, CAPTURE, CLOSE};

class V4LCapture : public HeadFilter {

public:
    V4LCapture();
    ~V4LCapture();
    
    bool configure(std::string device, unsigned width, unsigned height, unsigned fps, std::string format = "YUYV", bool fFormat = true);
    bool releaseDevice();
    
protected:
    virtual bool doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret);
    virtual FrameQueue *allocQueue(ConnectionData cData);
    virtual void doGetState(Jzon::Object &filterNode);

private:
    //NOTE: There is no need of specific writer configuration
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};
    
    const bool getFrame(std::chrono::microseconds timeout, VideoFrame *dstFrame);
    
    bool init_mmap();

    bool openDevice(std::string device);
    void closeDevice();

    bool initDevice(unsigned& xres, unsigned& yres, unsigned &den, std::string &format);
    bool uninitDevice();

    bool startCapturing();
    bool stopCapturing();

    bool readFrame(VideoFrame* dstFrame);

    StreamInfo* oStreamInfo;
    
    std::string device;
    
    struct v4l2_format fmt;
    DeviceStatus status;
    
    int fd;

    struct buffer   *buffers;
    unsigned        n_buffers;

    bool forceFormat;
    
    std::chrono::microseconds frameDuration;
    std::chrono::microseconds diff;
    std::chrono::high_resolution_clock::time_point wallclock;
    std::chrono::high_resolution_clock::time_point currentTime;
    
    unsigned frameCount;
};

#endif
