/*
 *  V4LCapture.cpp - Capture based on V4L2
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

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "V4LCapture.hh"
#include "../../AVFramedQueue.hh"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define TOLERANCE_FACTOR 32

static int xioctl(int fh, unsigned long int request, void *arg);
static PixType pixelType(int pixelFormat);

V4LCapture::V4LCapture() : HeadFilter(1, REGULAR, true), status(CLOSE), forceFormat(true)
{
    oStreamInfo = new StreamInfo (VIDEO);
    oStreamInfo->video.codec = RAW;
    oStreamInfo->video.pixelFormat = YUYV422;
}

V4LCapture::~V4LCapture()
{
    releaseDevice();
}

bool V4LCapture::configure(std::string device, unsigned width, unsigned height, unsigned fps, bool fFormat)
{
    forceFormat = fFormat;
    switch (status) {
        case CLOSE:
            if (!openDevice(device)){
                return false;
            }
        case OPEN:
            if (!initDevice(width, height, fps)){
                releaseDevice();
                return false;
            }
        case INIT:
            if (!startCapturing()){
                releaseDevice();
                return false;
            }
        case CAPTURE:
            if ((width != fmt.fmt.pix.width || 
                height != fmt.fmt.pix.height)){
                if (stopCapturing() && uninitDevice()){
                    return configure(device, width, height, fFormat);
                }
                return false;
            }
            break;
    }
    
    if (fps > 0){
        frameDuration = std::chrono::microseconds((int)(std::micro::den/fps));
    } else {
        frameDuration = std::chrono::microseconds((int)(std::micro::den/VIDEO_DEFAULT_FRAMERATE));
    }
    
    return true;
}

bool V4LCapture::releaseDevice()
{
    switch (status) {
        case CAPTURE:
            if (!stopCapturing()){
                return false;
            }
        case INIT:
            if (!uninitDevice()){
                return false;
            }
        case OPEN:
            closeDevice();
        case CLOSE:
            return true;
    }
    
    return true;
}

bool V4LCapture::doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret)
{   
    VideoFrame* frame = dynamic_cast<VideoFrame*> (dstFrames.begin()->second);
    
    if (!frame || status != CAPTURE){
        ret = WAIT;
        return false;
    }
    
    diff = std::chrono::duration_cast<std::chrono::microseconds> (std::chrono::high_resolution_clock::now() - wallclock);
    if (std::abs(diff.count()) > frameDuration.count()/TOLERANCE_FACTOR){
        utils::warningMsg("Resetting wallclock");
        wallclock = std::chrono::high_resolution_clock::now();
    }
    
    wallclock += frameDuration;
    
    if (!getFrame(frameDuration, frame)){
        frame->setConsumed(false);
        ret = 0;
        return false;
    }
    
    currentTime = std::chrono::high_resolution_clock::now();
    
    frame->setConsumed(true);
    frame->setPresentationTime(std::chrono::duration_cast<std::chrono::microseconds>(
        currentTime.time_since_epoch()));
    
    ret = (std::chrono::duration_cast<std::chrono::microseconds> (
        wallclock - currentTime)).count();
    
    return true;
}

FrameQueue* V4LCapture::allocQueue(ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, oStreamInfo, DEFAULT_RAW_VIDEO_FRAMES);
}

const bool V4LCapture::getFrame(std::chrono::microseconds timeout, VideoFrame *dstFrame)
{
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    tv.tv_sec = timeout.count()/std::micro::den;
    tv.tv_usec = timeout.count()%std::micro::den;

    ret = select(fd + 1, &fds, NULL, NULL, &tv);

    if (ret < 0) {
        utils::errorMsg("Select error");
        return false;
    }

    if (ret == 0) {
        utils::warningMsg("Select timeout ");
        return false;
    }

    return readFrame(dstFrame);
}

bool V4LCapture::readFrame(VideoFrame *dstFrame)
{

    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        return false;
    }
    
    memcpy(dstFrame->getDataBuf(), buffers[buf.index].data, 
               fmt.fmt.pix.height * fmt.fmt.pix.bytesperline);
    dstFrame->setSize(fmt.fmt.pix.width, fmt.fmt.pix.height);
    dstFrame->setPixelFormat(oStreamInfo->video.pixelFormat);
    
    if (xioctl(fd, VIDIOC_QBUF, &buf) < 0){
        return false;
    }

    return true;
}

bool V4LCapture::openDevice(std::string device)
{
      struct stat st;
      if (status != CLOSE){
          return false;
      }

      if (stat(device.c_str(), &st) < 0) {
          utils::errorMsg("Cannot find or identify " + device);
          return false;
      }

      if (!S_ISCHR(st.st_mode)) {
            utils::errorMsg(device + " is no device");
            return false;
      }

      fd = open(device.c_str(), O_RDWR | O_NONBLOCK, 0);

      if (fd < 0) {
            utils::errorMsg("Cannot open " + device);
            return false;
      }
      
      status = OPEN;
      return true;
}

bool V4LCapture::init_mmap(void)
{
      struct v4l2_requestbuffers req;

      CLEAR(req);

      req.count = 4;
      req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      req.memory = V4L2_MEMORY_MMAP;

      if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
          utils::errorMsg("Failed requesting v4l buffers");
          return false;
      }

      if (req.count < 2) {
          utils::errorMsg("Insuficient memory on ");
          return false;
      }

      buffers = (buffer*) calloc(req.count, sizeof(*buffers));

      if (!buffers) {
          utils::errorMsg("Out of memory");
          return false;
      }

      for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = n_buffers;

            if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0){
                utils::errorMsg("Failed requesting v4l buffers");
                return false;
            }

            buffers[n_buffers].size = buf.length;
            buffers[n_buffers].data =
                  mmap(NULL,
                        buf.length,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd, buf.m.offset);

            if (MAP_FAILED == buffers[n_buffers].data){
                  utils::errorMsg("Mmap failed");
                  return false;
            }
      }
      
      return true;
}

void V4LCapture::closeDevice(void)
{
    if (status != OPEN){
        return;
    }
    if (close(fd) < 0){
        utils::errorMsg("Failed closing file descriptor");
    }
    fd = -1;
    status = CLOSE;
}

bool V4LCapture::initDevice(unsigned& xres, unsigned& yres, unsigned& den)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_streamparm fps;
    
    if (status != OPEN){
        return false;
    }
    
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        utils::errorMsg("Device might not be V4L2 compatible");
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        utils::errorMsg("Device is no video capture device");
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        utils::errorMsg("Device does not support streaming i/o");
        return false;
    }

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; 
        
        xioctl(fd, VIDIOC_S_CROP, &crop);
    }
    
    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (forceFormat) {
        fmt.fmt.pix.width       = xres;
        fmt.fmt.pix.height      = yres;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        
        if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0){
            utils::errorMsg("Error setting format");
            return false;
        }

        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV){
            utils::warningMsg("Webcam does not support YUYV format!");
        }

        if (fmt.fmt.pix.width != xres || fmt.fmt.pix.height != yres){
            utils::warningMsg("Requested resolution could not be set, is set to: \n\t\t" 
                + std::to_string(fmt.fmt.pix.width) + " xres\n\t\t"
                + std::to_string(fmt.fmt.pix.height) + " yres");
            xres = fmt.fmt.pix.width;
            yres = fmt.fmt.pix.height;
        }
        
    } else {    
        if (xioctl(fd, VIDIOC_G_FMT, &fmt) < 0){
            utils::errorMsg("Error setting format");
            return false;
        }
    }
    
    oStreamInfo->video.pixelFormat = pixelType(fmt.fmt.pix.pixelformat);
    
    CLEAR(fps);
    fps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    fps.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
    fps.parm.capture.timeperframe.numerator = 1;
    fps.parm.capture.timeperframe.denominator = den;
    if (xioctl(fd, VIDIOC_S_PARM, &fps) < 0) {
        utils::errorMsg("Error setting fps!");
        return false;
    }
    
    if (fps.parm.capture.timeperframe.denominator != den){
        utils::warningMsg("Couldn't set fps to " + std::to_string(den) + ". Set to " 
            + std::to_string(fps.parm.capture.timeperframe.denominator));
        den = fps.parm.capture.timeperframe.denominator;
    }

    if (!init_mmap()){
        return false;
    }
    
    status = INIT;
    
    return true;
}

bool V4LCapture::uninitDevice(void)
{
    if (status != INIT){
        return false;
    }

    for (unsigned i = 0; i < n_buffers; ++i){
        if (munmap(buffers[i].data, buffers[i].size) < 0){
            utils::errorMsg("Failed unmapping buffers!");
            return false;
        }
    }

    CLEAR(fmt);
    free(buffers);
    status = OPEN;
    return true;
}

bool V4LCapture::startCapturing(void)
{
    enum v4l2_buf_type type;
    
    if (status != INIT){
        return false;
    }

    for (unsigned i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QBUF, &buf) < 0){
            utils::errorMsg("VIDIOC_QBUF error");
            return false;
        }
    }
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0){
        utils::errorMsg("VIDIOC_STREAMON error");
        return false;
    }
    
    status = CAPTURE;
    
    return true;
}

bool V4LCapture::stopCapturing(void)
{
    enum v4l2_buf_type type;
    
    if (status != CAPTURE){
        return false;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
        utils::errorMsg("VIDIOC_STREAMOFF");
        return false;
    }
    
    status = INIT;
    
    return true;
}

void V4LCapture::doGetState(Jzon::Object &filterNode)
{
    
}

static int xioctl(int fh, unsigned long int request, void *arg)
{
      int r;
      do {
            r = ioctl(fh, request, arg);
      } while (-1 == r && EINTR == errno);

      return r;
}

static PixType pixelType(int pixelFormat)
{
    switch(pixelFormat){
        case V4L2_PIX_FMT_YUYV:
            return YUYV422;
            break;
        case V4L2_PIX_FMT_YUV420:
            return YUV420P;
            break;
        default:
            utils::warningMsg("Unknown pixelFormat!");
            return P_NONE;
            break;
    }
}
