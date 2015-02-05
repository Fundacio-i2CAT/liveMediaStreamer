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

extern "C" {
    #include "i2libdash.h"
}

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

#define MICROSECONDS_TIME_BASE 1000000

class DashSegmenter;
class DashVideoSegmenter;
class DashAudioSegmenter;
class DashSegment;

class Dasher : public TailFilter {

public:
    Dasher(int segDur, int readersNum = MAX_READERS);
    ~Dasher();

    bool deleteReader(int id);
    void doGetState(Jzon::Object &filterNode);
      
private: 
    bool doProcessFrame(std::map<int, Frame*> orgFrames);
    void initializeEventMap();
    Reader *setReader(int readerId, FrameQueue* queue, bool sharedQueue = false);

    std::map<int, DashSegmenter*> segmenters;
    int segmentDuration;
};

class DashSegmenter {

public:
    DashSegmenter(size_t segDur);
    virtual bool manageFrame(Frame* frame) = 0;

protected:
    i2ctx* dashContext;
    size_t timeBase;
    size_t segmentDuration;
    size_t frameDuration;
    DashSegment* segment;
    DashSegment* initSegment;
    std::string initPath;
};

class DashVideoSegmenter : public DashSegmenter {

public:
    DashVideoSegmenter(size_t segDur);
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
    bool appendFrameToDashSegment(size_t pts, bool isIntra);
    bool updateTimeValues(size_t currentTimestamp);
    bool generateInit();

    std::vector<unsigned char> frameData;
    std::vector<unsigned char> lastSPS;
    std::vector<unsigned char> lastPPS;
    std::vector<unsigned char> metadata;
    bool updatedSPS;
    bool updatedPPS;
    size_t lastTs;
    size_t frameRate;
    bool isIntra;

    int frameCounter;

};

class DashAudioSegmenter : public DashSegmenter {

public:
    DashAudioSegmenter(size_t segDur);
    bool manageFrame(Frame* frame);
    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample);

};

/*! It represents a dash segment. It contains a buffer with the segment data (it allocates data) and its length. Moreover, it contains the
    segment sequence number and the output file name (in order to write the segment to disk) */ 

class DashSegment {
    
public:
    /**
    * Class constructor
    * @param maxSize Segment data max length
    */ 
    DashSegment(size_t maxSize);

    /**
    * Class destructor
    */ 
    ~DashSegment();

    /**
    * @return Pointer to segment data
    */ 
    unsigned char* getDataBuffer() {return data;};

    /**
    * @return Segment data length in bytes
    */ 
    size_t getDataLength() {return dataLength;};

    /**
    * @params Segment data length in bytes
    */ 
    void setDataLength(size_t length);

    /**
    * It sets the sequence number of the segment
    * @params seqNum is the sequence number to set
    */ 
    void setSeqNumber(size_t seqNum);
    
    /**
    * @return Segment sequence number
    */ 
    size_t getSeqNumber(){return seqNumber;};

    /**
    * Increment by one the sequence number
    */ 
    void incrSeqNumber(){seqNumber++;};

    /**
    * @return Segment segment timestamp
    */ 
    size_t getTimestamp(){return timestamp;};

    /**
    * It sets the segment timestamp
    * @params ts is the timestamp to set
    */ 
    void setTimestamp(size_t ts);

    /**
    * Writes segment to disk
    * @params Path to write
    */ 
    void writeToDisk(std::string path);

    /**
    * Clears segment data
    */ 
    void clear();
    
    /**
    * @return true if the segment has no data
    */ 
    bool isEmpty() {return (dataLength == 0 && seqNumber == 0 && timestamp == 0);};

private:
    unsigned char* data;
    size_t dataLength;
    size_t seqNumber;
    size_t timestamp;
};

#endif
