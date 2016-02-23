/*
 *  VideoCapture.hh - Video capture filter based on OpenCV
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

#ifndef _VIDEO_CAPTURE_HH
#define _VIDEO_CAPTURE_HH

#include <chrono>
#include<opencv2/opencv.hpp>

#include "../../Filter.hh"

/** Video capture module based on OpenCV
  * 
  * It produces one writer for each stream contained in the muxed file.
  * Use #BaseFilter::getState() to retrieve the description of the streams and their
  * writerId so you can connect further filters.
  */
class VideoCapture : public HeadFilter {

public:
    VideoCapture();
    ~VideoCapture();
    
    bool configure(int cam, int width, int height, float fps);
    void releaseDevice();
    
protected:
    virtual bool doProcessFrame(std::map<int, Frame*> &dstFrames, int& ret);
    virtual FrameQueue *allocQueue(ConnectionData cData);
    virtual void doGetState(Jzon::Object &filterNode);
    
private:
    
    //NOTE: There is no need of specific writer configuration
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};
    
    cv::VideoCapture device;
    StreamInfo* oStreamInfo;
    
    int camera;
    
    std::chrono::microseconds frameDuration;
    std::chrono::microseconds diff;
    std::chrono::high_resolution_clock::time_point wallclock;
    std::chrono::high_resolution_clock::time_point currentTime;
    //std::chrono::high_resolution_clock::time_point lastTimePoint;
    //std::chrono::high_resolution_clock::time_point tmpTimePoint;
    
    cv::Mat img;
};

#endif
