/*
 *  DashVideoSegmenterHEVC.hh - DASH HEVC video stream segmenter
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

#ifndef _DASH_VIDEO_SEGMENTER_HEVC_HH
#define _DASH_VIDEO_SEGMENTER_HEVC_HH

#include "Dasher.hh"
#include "DashVideoSegmenter.hh"

#define H265_NALU_TYPE_MASK 0x7E
#define H265_METADATA_VERSION_FLAG 0x01
#define H265_METADATA_CONFIG_FLAGS 0x01
#define H265_METADATA_PROFILE_COMPATIBILITY_FLAGS 0x60
#define H265_METADATA_PROFILE_COMPATIBILITY_FLAGS_PADDING 3 
#define H265_METADATA_CONSTRAINT_INDICATOR_FLAGS 0x90
#define H265_METADATA_CONSTRAINT_INDICATOR_FLAGS_PADDING 5 
#define H265_METADATA_GENERAL_LEVEL_IDC 0x5D //93
#define H265_METADATA_MIN_SPATIAL_SEGMENTATION_RESERVED_BYTES 0xF0
#define H265_METADATA_MIN_SPATIAL_SEGMENTATION 0
#define H265_METADATA_PARALLELISM_TYPE_RESERVED_BYTES 0xFC
#define H265_METADATA_PARALLELISM_TYPE 0
#define H265_METADATA_CHROMA_FORMAT_RESERVED_BYTES 0XF8
#define H265_METADATA_CHROMA_FORMAT 1
#define H265_METADATA_BIT_DEPTH_LUMA_MINUS_8_RESERVED_BYTES 0XF8
#define H265_METADATA_BIT_DEPTH_LUMA_MINUS_8 0
#define H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8_RESERVED_BYTES 0XF8
#define H265_METADATA_BIT_DEPTH_CHROMA_MINUS_8 0

#define H265_METADATA_AVG_FRAMERATE 0 
#define H265_METADATA_CTX_FRAMERATE 0
#define H265_METADATA_NUM_TEMPORAL_LAYERS 1
#define H265_METADATA_TEMPORAL_ID_NESTED 1
#define H265_METADATA_LENGTH_SIZE_MINUS_ONE 3

#define H265_NUMBER_OF_ARRAYS 3
#define H265_NUM_NALUS_IN_ARRAY 1

#define NON_TSA_STSA_0 0
#define NON_TSA_STSA_1 1
#define IDR1 19
#define IDR2 20
#define CRA 21
#define VPS 32
#define SPS_HEVC 33
#define PPS_HEVC 34
#define AUD_HEVC 35
#define PREFIX_SEI_NUT 39
#define SUFFIX_SEI_NUT 40 

/*! Class responsible for managing DASH HEVC video segments creation. It receives H265 NALs, joining them into complete frames
    and using these frames to create the segments. It also manages Init Segment creation, constructing MP4 metadata from
    VPS, SPS and PPS NALUs*/

class DashVideoSegmenterHEVC : public DashVideoSegmenter {

public:
    /**
    * Class constructor
    * @param segDur Segment duration in milliseconds
    * @param offset of the initial timestamp
    */
    DashVideoSegmenterHEVC(std::chrono::seconds segDur);

    /**
    * Class destructor
    */
    ~DashVideoSegmenterHEVC();

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
    void saveVPS(unsigned char* data, int dataLength);
    void saveSPS(unsigned char* data, int dataLength);
    void savePPS(unsigned char* data, int dataLength);
    void createExtradata();
    uint8_t generateContext();
    VideoFrame* parseNal(VideoFrame* nal);
    void resetFrame(){tmpFrame->setLength(0);};

    InterleavedVideoFrame* vFrame;
    InterleavedVideoFrame* tmpFrame;

    std::vector<unsigned char> vps;
    std::vector<unsigned char> sps;
    std::vector<unsigned char> pps;
};

#endif
