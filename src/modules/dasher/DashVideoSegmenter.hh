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

#define NON_IDR 1
#define IDR 5
#define SEI 6
#define SPS 7
#define PPS 8
#define AUD 9

/*! Class responsible for managing DASH video segments creation. It receives H264 NALs, joining them into complete frames
    and using these frames to create the segments. It also manages Init Segment creation, constructing MP4 metadata from
    SPS and PPS NALUs*/ 

class DashVideoSegmenter : public DashSegmenter {

public:
    /**
    * Class constructor
    * @param segDur Segment duration in milliseconds 
    * @param segBaseName Base name for the segments. Segment names will be: segBaseName_<timestamp>.m4v and segBaseName_init.m4v
    */ 
    DashVideoSegmenter(size_t segDur, std::string segBaseName);

    /**
    * Class destructor
    */ 
    ~DashVideoSegmenter();

    /**
    * It manages an input NAL, doing different actions depending on its type. Contemplated NALUs are:
    *   - SPS (7) and PPS (8) 
    *      Saves this NALs to generate InitSegment. They will be not appended to the frame
    *   - SEI (6)
    *       It is just ignored
    *   - AUD (9)
    *       It is used to detect the end of a frame. It is not appended to the internal frame buffer. It
    *       sets newFrame to true only if the previously received NALs are VCL (IDR and NON-IDR) 
    *   - IDR (5)
    *       It is appended to the internal frame buffer and activates isIntra flag
    *   - NON-IDR (1)
    *       It is appended to the internal frame buffer
    * @param frame Pointer the source NAL, which must be contained in a VideoFrame structure
    * @param newFrame Passed as reference, it indicates if there is a complete frame in the internal frame buffer
    * @return false if error and true if the NAL has been managed correctly
    */
    bool manageFrame(Frame* frame, bool &newFrame);

    /**
    * It setups internal stuff using internal information, set by manageFrame method
    * @return true if succeeded and false if not
    */
    bool updateConfig();

    /**
    * It creates a DASH video segment using the remaining data in the segment internal buffer. The duration of this segment
    * can be less than the defined segment duration, set on the constructor
    * @return true if succeeded and false if not
    */
    bool finishSegment();
    
    /**
    * Returns the isIntra flag, set by the last execution of manageFrame method
    * @return true if intraFrame and false if not
    */
    bool isIntraFrame() {return isIntra;};

    /**
    * Returns the isVCL flag, set by the last execution of manageFrame method
    * @return true if VCL NAL (IDR and NON-IDR) and false if not (SPS, PPS, SEI)
    */
    bool isVCLFrame() {return isVCL;};

    /**
    * Return the last stored SPS size
    * @return size in bytes
    */
    size_t getSPSsize() {return lastSPS.size();};

    /**
    * Return the last stored PPS size
    * @return size in bytes
    */
    size_t getPPSsize() {return lastPPS.size();};

    /**
    * Return the last stored PPS size
    * @return size in bytes
    */
    size_t getFrameDataSize() {return frameData.size();};

    /**
    * Return the presentation timestamp of the last NALU managed by manageFrame method
    * @return timestamp in milliseconds
    */
    size_t getCurrentTimestamp() {return currTimestamp;};

    /**
    * Return the width of the last complete frame managed by manageFrame method
    * @return width in pixels
    */
    size_t getWidth() {return width;};

    /**
    * Return the height of the last complete frame managed by manageFrame method
    * @return height in pixels
    */
    size_t getHeight() {return height;};

    /**
    * Return the previous timestamp of the last NALU managed by manageFrame method
    * @return timestamp in milliseconds
    */
    size_t getLastTs() {return lastTs;};

    /**
    * Return the tiemstamp offset, which corresponds to the timestamp of the first NALU 
    * managed by manageFrame method. Each timestamp will be relative to this one.
    * @return timestamp in milliseconds
    */
    size_t getTsOffset() {return tsOffset;};

    /**
    * Return the tiemstamp offset, which corresponds to the timestamp of the first NALU 
    * managed by manageFrame method. Each timestamp will be relative to this one.
    * @return timestamp in milliseconds
    */
    size_t getFramerate() {return frameRate;};

private:
    bool updateMetadata();
    bool generateInitData();
    bool appendFrameToDashSegment();

    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate);
    bool parseNal(VideoFrame* nal, bool &newFrame);
    int detectStartCode(unsigned char const* ptr);
    void saveSPS(unsigned char* data, int dataLength);
    void savePPS(unsigned char* data, int dataLength);
    void createMetadata();
    bool appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, bool &newFrame);
    bool updateTimeValues();
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
