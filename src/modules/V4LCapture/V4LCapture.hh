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

#define TOLERANCE_FACTOR 64
#define FRAME_AVG_COUNT 128

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

private:
    bool doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret);
    FrameQueue *allocQueue(ConnectionData cData);
    
    void doGetState(Jzon::Object &filterNode);
    void initializeEventMap();
    bool configureEvent(Jzon::Node* params);

    bool specificWriterConfig(int /*writerID*/);
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
    
    int getAvgFrameDuration(std::chrono::microseconds duration);

    StreamInfo* oStreamInfo;
    
    std::string device;
    
    struct v4l2_format fmt;
    DeviceStatus status;
    
    int fd;

    struct buffer   *buffers;
    unsigned        n_buffers;

    bool forceFormat;
    
    unsigned frameCount;
    
    std::chrono::microseconds frameDuration;
    std::chrono::microseconds durationCount;
    std::chrono::high_resolution_clock::time_point wallclock;
    std::chrono::high_resolution_clock::time_point currentTime;
    std::chrono::high_resolution_clock::time_point lastTime;
};

#endif
