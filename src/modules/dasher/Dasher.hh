/*
 *  Dasher.hh - Class that handles DASH sessions
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *            
 */
 
#ifndef _DASHER_HH
#define _DASHER_HH

#include "../../Filter.hh"
#include "../../VideoFrame.hh"

#include <map>
#include <string>

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

class DashSegmenter;
class DashVideoSegmenter;
class DashAudioSegmenter;

class Dasher : public TailFilter {

public:
    Dasher(int readersNum = MAX_READERS);
    ~Dasher();

    bool deleteReader(int id);
    void doGetState(Jzon::Object &filterNode);
      
private: 
    bool doProcessFrame(std::map<int, Frame*> orgFrames);
    void initializeEventMap();
    Reader *setReader(int readerId, FrameQueue* queue, bool sharedQueue = false);


    std::map<int, DashSegmenter*> segmenters;
};

class DashSegmenter {

public:
    DashSegmenter();
    virtual bool manageFrame(Frame* frame) = 0;

protected:
    std::vector<unsigned char> frameData;
    // i2ctx* dashContext;

};

class DashVideoSegmenter : public DashSegmenter {

public:
    DashVideoSegmenter();
    bool manageFrame(Frame* frame);
    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate);

private:
    bool parseNal(VideoFrame* nal);
    int detectStartCode(unsigned char const* ptr);
    void saveSPS(unsigned char* data, int dataLength);
    void savePPS(unsigned char* data, int dataLength);
    bool updateMetadata();
    void createMetadata();
    bool appendNalToFrame(unsigned char* nalData, unsigned nalDataLength);


    std::vector<unsigned char> lastSPS;
    std::vector<unsigned char> lastPPS;
    std::vector<unsigned char> metadata;
    bool updatedSPS;
    bool updatedPPS;
};

class DashAudioSegmenter : public DashSegmenter {

public:
    DashAudioSegmenter();
    bool manageFrame(Frame* frame);
    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample);

};

#endif
