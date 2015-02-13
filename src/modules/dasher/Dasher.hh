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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            
 */
 
#ifndef _DASHER_HH
#define _DASHER_HH

#include "../../Filter.hh"
#include "../../VideoFrame.hh"
#include "../../AudioFrame.hh"

extern "C" {
    #include "i2libdash.h"
}

#include <map>
#include <string>

#define MICROSECONDS_TIME_BASE 1000000

class DashSegmenter;
class DashAudioSegmenter;
class DashSegment;

class Dasher : public TailFilter {

public:
    Dasher(int readersNum = MAX_READERS);
    ~Dasher();

    bool deleteReader(int id);
    void doGetState(Jzon::Object &filterNode);
    bool addSegmenter(int readerId, std::string segBaseName, int segDurInMicroSeconds);
    bool removeSegmenter(int readerId);
      
private: 
    bool doProcessFrame(std::map<int, Frame*> orgFrames);
    void initializeEventMap();

    std::map<int, DashSegmenter*> segmenters;
};

class DashSegmenter {

public:
    DashSegmenter(size_t segDur, size_t tBase, std::string segBaseName, std::string segExt);
    virtual ~DashSegmenter();
    virtual bool manageFrame(Frame* frame, bool &newFrame) = 0;
    virtual bool updateConfig() = 0;
    virtual bool finishSegment() = 0;
    bool generateInitSegment();
    bool generateSegment();
    size_t getFrameDuration() {return frameDuration;};
    size_t getTimeBase() {return timeBase;};
    size_t getTsOffset() {return tsOffset;};

protected:
    virtual bool updateMetadata() = 0;
    virtual bool generateInitData() = 0;
    virtual bool appendFrameToDashSegment() = 0;
    std::string getInitSegmentName();
    std::string getSegmentName();
    
    i2ctx* dashContext;
    size_t timeBase;
    size_t segmentDuration;
    size_t frameDuration;
    DashSegment* segment;
    DashSegment* initSegment;
    std::string baseName;
    std::string segmentExt;
    std::vector<unsigned char> metadata;
    size_t tsOffset;
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
    * Increase by one the sequence number
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
    * @retrun True if suceeded and False if not
    */ 
    bool writeToDisk(std::string path);

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
