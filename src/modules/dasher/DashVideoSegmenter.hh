/*
 *  DashVideoSegmenter.hh - DASH video stream segmenter
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
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
 *
 */

#ifndef _DASH_VIDEO_SEGMENTER_HH
#define _DASH_VIDEO_SEGMENTER_HH

#include "Dasher.hh"

#define H264or5_NALU_START_CODE 0x00000001
#define SHORT_START_CODE_LENGTH 3
#define LONG_START_CODE_LENGTH 4

/*! Virtual class responsible for managing DASH video segments creation. It receives H264or5 NALs, joining them into complete frames
    and using these frames to create the segments. It also manages Init Segment creation, constructing MP4 metadata from
    SPS and PPS (and VPS) NALUs*/

class DashVideoSegmenter : public DashSegmenter {

public:
    /**
    * It manages an input NAL, doing different actions depending on its type. See children classes headers to check which 
    * types of NALUs are checked.
    * @param frame Pointer the source NAL, which must be contained in a VideoFrame structure
    * @param newFrame Passed as reference, it indicates if there is a complete frame in the internal frame buffer
    * @return false if error and true if the NAL has been managed correctly
    */
    bool manageFrame(Frame* frame, bool &newFrame);

    size_t getFrameDataSize() {return vFrame->getLength();};

    /**
    * Return the presentation timestamp of the last NALU managed by manageFrame method
    * @return timestamp in milliseconds
    */
    std::chrono::microseconds getCurrentTimestamp() {return vFrame->getPresentationTime();};

    /**
    * Return the width of the last complete frame managed by manageFrame method
    * @return width in pixels
    */
    size_t getWidth() {return vFrame->getWidth();};

    /**
    * Return the height of the last complete frame managed by manageFrame method
    * @return height in pixels
    */
    size_t getHeight() {return vFrame->getHeight();};

    /**
    * Return the framerate, which is calculated by the difference between currtimestamp and lastTs
    * @return framerate in frames per second
    */
    size_t getFramerate() {return frameRate;};

    /**
    * Return the video format string (i.e.: avc or hevc types)
    * @return video format as string
    */
    std::string getVideoFormat() {return video_format;};

    /**
    * Processes incoming frames to be appended to current segment
    * @return true if success, false otherwise.
    */
    bool appendFrameToDashSegment(DashSegment* segment);

    /**
    * Virtual method that flushes segment context at children classes
    * @return true if success, false otherwise.
    */
    virtual bool flushDashContext() = 0;

    bool isIntraFrame() {return isIntra;};

protected:
    DashVideoSegmenter(std::chrono::seconds segDur, std::string video_format_);
    virtual ~DashVideoSegmenter();

    virtual bool updateMetadata() = 0;
    virtual bool appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, bool &newFrame) = 0;
    virtual void createMetadata() = 0;
    virtual uint8_t generateContext() = 0;

    int detectStartCode(unsigned char const* ptr);
    bool setup(size_t segmentDuration, size_t timeBase, size_t width, size_t height);
    bool generateInitData(DashSegment* segment);
    bool parseNal(VideoFrame* nal, bool &newFrame);
    unsigned customGenerateSegment(unsigned char *segBuffer, unsigned &segTimestamp, unsigned &segDuration, bool force);

    InterleavedVideoFrame* vFrame;

    size_t frameRate;
    bool isIntra;
    const std::string video_format;
};

#endif
