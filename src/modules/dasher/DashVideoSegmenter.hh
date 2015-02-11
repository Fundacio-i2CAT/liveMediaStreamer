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

#define H264_NALU_START_CODE 0x00000001
#define SHORT_START_CODE_LENGTH 3
#define LONG_START_CODE_LENGTH 4
#define H264_NALU_TYPE_MASK 0x1F

#define H264_METADATA_VERSION_FLAG 0x01
#define METADATA_RESERVED_BYTES1 0xFC
#define AVCC_HEADER_BYTES_MINUS_ONE 0x03
#define METADATA_RESERVED_BYTES2 0xE0
#define NUMBER_OF_SPS 0x01
#define NUMBER_OF_PPS 0x01

#define IDR 5
#define SEI 6
#define SPS 7
#define PPS 8
#define AUD 9

class DashVideoSegmenter : public DashSegmenter {

public:
    DashVideoSegmenter(size_t segDur, std::string segBaseName);
    ~DashVideoSegmenter();
    bool manageFrame(Frame* frame);
    bool updateConfig();
    bool finishSegment();

private:
    bool updateMetadata();
    bool generateInitData();
    bool appendFrameToDashSegment();

    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate);
    bool parseNal(VideoFrame* nal);
    int detectStartCode(unsigned char const* ptr);
    void saveSPS(unsigned char* data, int dataLength);
    void savePPS(unsigned char* data, int dataLength);
    void createMetadata();
    bool appendNalToFrame(unsigned char* nalData, unsigned nalDataLength);
    void updateTimeValues();
    size_t customTimestamp(size_t currentTimestamp);

    std::vector<unsigned char> frameData;
    std::vector<unsigned char> lastSPS;
    std::vector<unsigned char> lastPPS;
    bool updatedSPS;
    bool updatedPPS;
    size_t lastTs;
    size_t tsOffset;
    size_t frameRate;
    bool isIntra;
    bool isVCL;
    size_t currTimestamp;
    size_t width;
    size_t height;

};

#endif
