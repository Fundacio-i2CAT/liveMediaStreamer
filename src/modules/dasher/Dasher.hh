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
#include "MpdManager.hh"

extern "C" {
    #include "i2libdash.h"
}

#include <map>
#include <string>

#define MICROSECONDS_TIME_BASE 1000000
#define DASH_VIDEO_TIME_BASE 12800
#define V_ADAPT_SET_ID "0"
#define A_ADAPT_SET_ID "1"
#define VIDEO_CODEC "avc1.42c01e"
#define AUDIO_CODEC "mp4a.40.2"
#define V_EXT ".m4v"
#define A_EXT ".m4a"

class DashSegmenter;
class DashSegment;

/*! Class responsible for managing DASH segmenters. */ 

class Dasher : public TailFilter {

public:
    static Dasher* createNew(std::string dashFolder, std::string baseName, size_t segDurInSeconds, std::string mpdLocation, int readersNum = MAX_READERS);
    /**
    * Class destructor
    */ 
    ~Dasher();

    //TODO: check this
    bool deleteReader(int id);

    
    void doGetState(Jzon::Object &filterNode);

    /**
    * Adds a new segmenter associated to an existance reader. Only one segmenter can be associated to each reader
    * @param readerId Reader associated to the segmenter 
    * @return true if suceeded and false if not
    */ 
    bool addSegmenter(int readerId);

    /**
    * Remvoes an existant segmenter, using its associated reader id. A segment with the remaining 
    * data buffered (if any) is created and written to disk
    * @param readerId Reader associated to the segmenter 
    * @return true if suceeded and false if not
    */ 
    bool removeSegmenter(int readerId);

    static std::string getSegmentName(std::string basePath, std::string baseName, size_t reprId, size_t timestamp, std::string ext);
    static std::string getInitSegmentName(std::string basePath, std::string baseName, size_t reprId, std::string ext);
      
private: 
    Dasher(std::string dashFolder, std::string baseName, size_t segDurInSec, std::string mpdLocation, int readersNum = MAX_READERS);
    bool doProcessFrame(std::map<int, Frame*> orgFrames);
    void initializeEventMap();
    void updateMpd(int id, DashSegmenter* segmenter);
    bool generateInitSegment(size_t id, DashSegmenter* segmenter);
    bool generateSegment(size_t id, DashSegmenter* segmenter);
    size_t updateTimestampControl(std::map<int,DashSegment*> segments);
    bool writeSegmentsToDisk(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt);
    bool cleanSegments(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt);


    std::map<int, DashSegmenter*> segmenters;
    std::map<int, DashSegment*> vSegments;
    std::map<int, DashSegment*> aSegments;
    std::map<int, DashSegment*> initSegments;

    MpdManager* mpdMngr;
    size_t timestampOffset;
    size_t segDurInMicrosec;
    std::string basePath;
    std::string baseName;
    std::string mpdPath;
    std::string segmentsBasePath;
    std::string vSegTempl;
    std::string aSegTempl;
    std::string vInitSegTempl;
    std::string aInitSegTempl;
};

/*! Abstract class implemented by DashVideoSegmenter and DashAudioSegmenter */ 

class DashSegmenter {

public:
    
    /**
    * Class constructor
    * @param segDur Segment duration in tBase units 
    * @param tBase segDur timeBase 
    * @param segBaseName Base name for the segments. Segment names will be: segBaseName_<timestamp>.segExt and segBaseName_init.segExt
    * @param segExt segment file extension (.m4v for video and .m4a for audio) 
    */ 
    DashSegmenter(size_t segDur, size_t tBase);

    /**
    * Class destructor
    */ 
    virtual ~DashSegmenter();

    /**
    * Implemented by DashVideoSegmenter::manageFrame and DashAudioSegmenter::manageFrame
    */ 
    virtual bool manageFrame(Frame* frame, bool &newFrame) = 0;

    /**
    * Implemented by DashVideoSegmenter::updateConfig and DashAudioSegmenter::updateConfig
    */ 
    virtual bool updateConfig() = 0;

    /**
    * Implemented by DashVideoSegmenter::finishSegment and DashAudioSegmenter::finishSegment
    */ 
    virtual bool finishSegment() = 0;

    /**
    * Generates and writes to disk an Init Segment if possible (or necessary)
    */ 
    bool generateInitSegment(DashSegment* segment);

    /**
    * Generates and writes to disk a segment if there is enough data buffered
    */ 
    bool generateSegment(DashSegment* segment);

    /**
    * Returns average frame duration 
    * @return duration in time base
    */
    size_t getFrameDuration() {return frameDuration;};

    /**
    * Returns the timestamp time base of the segments 
    * @return time base in ticks per second
    */
    size_t getTimeBase() {return timeBase;};

    /**
    * Return the timestamp offset, which corresponds to the timestamp of the first frame 
    * managed by manageFrame method. Each timestamp will be relative to this one.
    * @return timestamp in milliseconds
    */
    size_t getTsOffset() {return tsOffset;};

    /**
    * It returns the last segment timestamp 
    * @return timestamp in timeBase units
    */
    size_t getSegmentTimestamp();
    void setOffset(size_t offs);

    /**
    * Return the segment duration 
    * @return segment duration in timeBase units
    */
    size_t getsegDurInMicroSec() {return segDurInMicroSec;};
    size_t getSegDurInTimeBaseUnits() {return segDurInTimeBaseUnits;};


protected:
    virtual bool updateMetadata() = 0;
    virtual bool generateInitData(DashSegment* segment) = 0;
    virtual bool appendFrameToDashSegment(DashSegment* segment) = 0;
    std::string getInitSegmentName();
    std::string getSegmentName();
    size_t customTimestamp(size_t timestamp);
    size_t microsToTimeBase(size_t microsValue);
    
    i2ctx* dashContext;
    size_t timeBase;
    size_t segDurInMicroSec;
    size_t segDurInTimeBaseUnits;
    size_t frameDuration;
    std::vector<unsigned char> metadata;
    size_t tsOffset;
    size_t theoricPts;
};

/*! It represents a dash segment. It contains a buffer with the segment data (it allocates data) and its length. Moreover, it contains the
    segment sequence number and the output file name (in order to write the segment to disk) */ 

class DashSegment {
    
public:
    /**
    * Class constructor
    * @param maxSize Segment data max length
    */ 
    DashSegment(size_t maxSize = MAX_DAT);

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
