/*
 *  VideoCapture.cpp - Video capture filter based on OpenCV
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

#include "VideoCapture.hh"
#include "../../VideoFrame.hh"
#include "../../AVFramedQueue.hh"

#include <cmath>

VideoCapture::VideoCapture() : HeadFilter(1, REGULAR, true), 
        camera(-1), 
        frameDuration(std::chrono::microseconds(0))
{
    oStreamInfo = new StreamInfo (VIDEO);
    oStreamInfo->video.codec = RAW;
    oStreamInfo->video.pixelFormat = RGB24;
}

VideoCapture::~VideoCapture()
{
}

bool VideoCapture::configure(int cam, int width, int height, float fps)
{
    if (camera != cam){
        releaseDevice();
    }
    
    device.open(cam);
    if (! device.isOpened()){
        camera = -1;
        return false;
    }
    
    camera = cam;
    
    if (width > 0){
        device.set(CV_CAP_PROP_FRAME_WIDTH, width);
        if (width != device.get(CV_CAP_PROP_FRAME_WIDTH)){
            utils::warningMsg("Desired with could not be set, set to " + 
                std::to_string(device.get(CV_CAP_PROP_FRAME_WIDTH)));
        }
    }
    
    if (height > 0){
        device.set(CV_CAP_PROP_FRAME_HEIGHT, height);
        if (height != device.get(CV_CAP_PROP_FRAME_HEIGHT)){
            utils::warningMsg("Desired height could not be set, set to " + 
                std::to_string(device.get(CV_CAP_PROP_FRAME_HEIGHT)));
        }
    }
    
    if (fps > 0){
        frameDuration = std::chrono::microseconds((int)(std::micro::den/fps));
    } else {
        frameDuration = std::chrono::microseconds((int)(std::micro::den/VIDEO_DEFAULT_FRAMERATE));
    }
    
    img = cv::Mat(device.get(CV_CAP_PROP_FRAME_HEIGHT), 
                  device.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3, NULL);
    
    wallclock = std::chrono::high_resolution_clock::now();
    
    return true;
}

void VideoCapture::releaseDevice()
{
    if (device.isOpened()){
        device.release();
    }
}

bool VideoCapture::doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret)
{   
    VideoFrame* frame = dynamic_cast<VideoFrame*> (dstFrames.begin()->second);
    
    if (!device.isOpened() || !frame){
        return false;
    }
    
    diff = std::chrono::duration_cast<std::chrono::microseconds> (wallclock - std::chrono::high_resolution_clock::now());
    if (std::abs(diff.count()) > frameDuration.count()/16){
        utils::warningMsg("Resetting wallclock");
        wallclock = std::chrono::high_resolution_clock::now();
    }
    
    wallclock += frameDuration;
    
    img.data = frame->getDataBuf();
    frame->setLength(img.step * device.get(CV_CAP_PROP_FRAME_HEIGHT));
    frame->setSize(device.get(CV_CAP_PROP_FRAME_WIDTH), 
                   device.get(CV_CAP_PROP_FRAME_HEIGHT));
    
    if (!device.read(img)){
        ret =  WAIT;
        return false;
    }
    
    frame->setConsumed(true);
    
    currentTime = std::chrono::high_resolution_clock::now();
    
    frame->setPresentationTime(std::chrono::duration_cast<std::chrono::microseconds>(
        currentTime.time_since_epoch()));
    
    ret = (std::chrono::duration_cast<std::chrono::microseconds> (
        wallclock - currentTime)).count();
    
    return true;
}

FrameQueue* VideoCapture::allocQueue(ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, oStreamInfo, DEFAULT_RAW_VIDEO_FRAMES);
}

void VideoCapture::doGetState(Jzon::Object &filterNode)
{
    
}
