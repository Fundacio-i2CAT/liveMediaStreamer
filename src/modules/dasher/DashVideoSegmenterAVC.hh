/*
 *  DashVideoSegmenterAVC.hh - DASH AVC video stream segmenter
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
 *  Authors:  Gerard Castillo <gerard.castillo@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net> 
 *
 */

#ifndef _DASH_VIDEO_SEGMENTER_AVC_HH
#define _DASH_VIDEO_SEGMENTER_AVC_HH

#include "Dasher.hh"
#include "DashVideoSegmenter.hh"

#define H264_NALU_TYPE_MASK 0x1F
#define H264_METADATA_VERSION_FLAG 0x01
#define METADATA_RESERVED_BYTES1 0xFC
#define METADATA_RESERVED_BYTES2 0xE0
#define NUMBER_OF_SPS 0x01
#define NUMBER_OF_PPS 0x01

#define NON_IDR 1
#define IDR 5
#define SEI 6
#define SPS_AVC 7
#define PPS_AVC 8
#define AUD_AVC 9

/*! Class responsible for managing DASH AVC video segments creation. It receives H264 NALs, joining them into complete frames
    and using these frames to create the segments. It also manages Init Segment creation, constructing MP4 metadata from
    SPS and PPS NALUs*/

class DashVideoSegmenterAVC : public DashVideoSegmenter {

public:
    /**
    * Class constructor
    * @param segDur Segment duration in milliseconds
    * @param offset of the initial timestamp
    */
    DashVideoSegmenterAVC(std::chrono::seconds segDur, std::chrono::microseconds& offset);

    /**
    * Class destructor
    */
    ~DashVideoSegmenterAVC();

    /**
    * Return the last stored SPS size
    * @return size in bytes
    */
    size_t getSPSsize() {return sps.size();};

    /**
    * Return the last stored PPS size
    * @return size in bytes
    */
    size_t getPPSsize() {return pps.size();};

    /**
    * Flushes segment context
    * @return true if success, false otherwise.
    */
    bool flushDashContext();
    size_t getFrameDataSize() {return tmpFrame->getLength();};
    

private:
    void updateExtradata();
    void saveSPS(unsigned char* data, int dataLength);
    void savePPS(unsigned char* data, int dataLength);
    void createExtradata();
    uint8_t generateContext();
    VideoFrame* parseNal(VideoFrame* nal);
    void resetFrame(){tmpFrame->setLength(0);};

    InterleavedVideoFrame* vFrame;
    InterleavedVideoFrame* tmpFrame;

    std::vector<unsigned char> sps;
    std::vector<unsigned char> pps;
};

#endif
