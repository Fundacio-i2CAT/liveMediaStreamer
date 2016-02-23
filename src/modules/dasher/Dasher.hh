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
 *            Gerard Castillo <gerard.castillo@i2cat.net>
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

#define DASH_VIDEO_TIME_BASE    12800
#define V_ADAPT_SET_ID          "0"
#define A_ADAPT_SET_ID          "1"
#define VIDEO_CODEC_AVC         "avc1.42c01e"
#define VIDEO_CODEC_HEVC        "hev1"
#define AUDIO_CODEC             "mp4a.40.2"
#define V_EXT                   ".m4v"
#define A_EXT                   ".m4a"

class DashSegmenter;
class DashSegment;

/*! Class responsible for managing DASH segmenters. */

class Dasher : public TailFilter {

public:
    /**
    * Constructor wrapper. It also configures the dasher filter with next required parameters:
    * @param dashFolder is the system folder where the segmenter is going to work with
    * @param baseName is the file base name for all generated and required files
    * @param segDurInSeconds is the time duration in seconds
    * @return Pointer to the object if succeded and NULL if not
    */
    Dasher(unsigned readersNum = MAX_READERS);

    /**
    * Class destructor
    */
    ~Dasher();

    //TODO: add documentation
    bool configure(std::string dashFolder, std::string baseName_, size_t segDurInSeconds, size_t maxSeg, size_t minBuffTime);

    /**
    * Creates a segment name as a function of the input and required params
    * @param basePath is the folder path where the segments are written
    * @param baseName is the base name for that segment specified as a convention
    * @param reprId is the specific ID of that segmenter with an associated reader
    * @param timestamp is the time stamp for that instant for that segment
    * @param ext is the file extension that indicates its file format
    * @return string of the segment name
    */
    static std::string getSegmentName(std::string basePath, std::string baseName, size_t reprId, size_t timestamp, std::string ext);

    /**
    * Creates the init segment name as a function of the input and required params
    * @param basePath is the folder path where the segments are written
    * @param baseName is the base name for that segment specified as a convention
    * @param reprId is the specific ID of that segmenter with an associated reader
    * @param ext is the file extension that indicates its file format
    * @return string of the initialization segment name
    */
    static std::string getInitSegmentName(std::string basePath, std::string baseName, size_t reprId, std::string ext);

    bool setDashSegmenterBitrate(int id, size_t kbps);

private:
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> newFrames, int& ret);
    void doGetState(Jzon::Object &filterNode);
    void initializeEventMap();
    bool generateInitSegment(size_t id, DashSegmenter* segmenter);
    bool generateSegment(size_t id, Frame* frame, DashSegmenter* segmenter);
    bool appendFrameToSegment(size_t id, Frame* frame, DashSegmenter* segmenter);
    DashSegmenter* getSegmenter(size_t id);
    bool forceAudioSegmentsGeneration();
    bool writeVideoSegments();
    bool writeAudioSegments();

    bool writeSegmentsToDisk(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt);
    bool cleanSegments(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt);
    bool configureEvent(Jzon::Node* params);

    bool setBitrateEvent(Jzon::Node* params);
    
    bool specificReaderConfig(int readerID, FrameQueue* queue);
    bool specificReaderDelete(int readerID);

    std::map<int, DashSegmenter*> segmenters;
    std::map<int, DashSegment*> vSegments;
    std::map<int, DashSegment*> aSegments;
    std::map<int, DashSegment*> initSegments;

    MpdManager* mpdMngr;
    std::chrono::seconds segDur;

    std::string basePath;
    std::string baseName;
    std::string mpdPath;
    std::string vSegTempl;
    std::string aSegTempl;
    std::string vInitSegTempl;
    std::string aInitSegTempl;

    bool hasVideo;
    bool videoStarted;
    
    std::chrono::microseconds timestampOffset;
};

/*! Abstract class implemented by DashVideoSegmenter and DashAudioSegmenter */

class DashSegmenter {

public:

    /**
    * Class constructor
    * @param segDur Segment duration in tBase units
    * @param tBase segDur timeBase
    * @param offset timestamp of the current dash session
    */
    DashSegmenter(std::chrono::seconds segmentDuration, size_t tBase, std::chrono::microseconds offset);

    /**
    * Class destructor
    */
    virtual ~DashSegmenter();

    /**
    * Implemented by DashVideoSegmenter::manageFrame and DashAudioSegmenter::manageFrame
    */
    virtual Frame* manageFrame(Frame* frame) = 0;

    /**
    * Generates and writes to disk an Init Segment if possible (or necessary)
    */
    virtual bool generateInitSegment(DashSegment* segment) = 0;

    virtual bool appendFrameToDashSegment(Frame* frame) = 0;
    bool generateSegment(DashSegment* segment, Frame* frame, bool force = false);
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
    * It returns the last segment timestamp
    * @return timestamp in timeBase units
    */
    size_t getSegmentTimestamp();

    /**
    * Sets the timestamp offset of the segmenter
    * @param microseconds timestamp in microseconds
    */
    void setOffset(std::chrono::microseconds offset) {tsOffset = offset;};
    
    /**
    * Return the segment duration
    * @return segment duration in timeBase units
    */
    size_t getSegDurInTimeBaseUnits() {return segDurInTimeBaseUnits;};
    
    virtual bool flushDashContext() = 0;

    void setBitrate(size_t bps) {bitrateInBitsPerSec = bps;};
    size_t getBitrate() {return bitrateInBitsPerSec;};

protected:
    virtual unsigned customGenerateSegment(unsigned char *segBuffer, std::chrono::microseconds nextFrameTs, 
                                            uint64_t &segTimestamp, uint32_t &segDuration, bool force) = 0;
    
    std::string getInitSegmentName();
    std::string getSegmentName();
    size_t customTimestamp(std::chrono::system_clock::time_point timestamp);
    size_t microsToTimeBase(std::chrono::microseconds microValue);

    std::chrono::seconds segDur;

    i2ctx* dashContext;
    size_t timeBase;
    size_t segDurInTimeBaseUnits;
    size_t frameDuration;
    size_t currentTimestamp;
    size_t sequenceNumber;
    std::vector<unsigned char> extradata;
    size_t bitrateInBitsPerSec;
    
    std::chrono::microseconds tsOffset;
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

    /**setSegmentsOffset
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
    * @return Segment segment timestamp
    */
    size_t getDuration(){return duration;};

    /**
    * It sets the segment timestamp
    * @params ts is the timestamp to set
    */
    void setDuration(size_t dur);

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

    bool isComplete() {return complete;};
    void setComplete(bool c) {complete = c;};

private:
    unsigned char* data;
    size_t dataLength;
    size_t seqNumber;
    size_t timestamp;
    size_t duration;
    bool complete;
};

#endif
