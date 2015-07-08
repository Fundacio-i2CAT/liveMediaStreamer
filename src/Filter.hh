/*
 *  Filter.hh - Filter base classes
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
 *            Marc Palau <marc.palau@i2cat.net>
 *	      Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _FILTER_HH
#define _FILTER_HH

#include <map>
#include <vector>
#include <queue>
#include <mutex>


#include "FrameQueue.hh"
#include "IOInterface.hh"
#include "Runnable.hh"
#include "Event.hh"


#define DEFAULT_ID 1                /*!< Default ID for unique filter's readers and/or writers. */
#define MAX_WRITERS 16              /*!< Default maximum writers for a filter. */
#define MAX_READERS 16              /*!< Default maximum readers for a filter. */
#define RETRY 500000                   /*!< Default retry time in nanoseconds (ns). */

/*! Generic filter class methods. It is an interface to different specific filters
    so it cannot be instantiated
*/
class BaseFilter : public Runnable {

public:
    /**
    * Creates a one to one connection from an available writer to an available reader
    * @param BaseFilter pointer of the filter to be connected
    * @return True if succeeded and false if not
    */
    bool connectOneToOne(BaseFilter *R);
    /**
    * Creates a many to one connection from specific writer to an available reader
    * @param BaseFilter pointer of the filter to be connected
    * @param Integer writer ID
    * @return True if succeeded and false if not
    */
    bool connectManyToOne(BaseFilter *R, int writerID);
    /**
    * Creates a one to many connection from an available writer to specific reader
    * @param BaseFilter pointer of the filter to be connected
    * @param Integer reader ID
    * @return True if succeeded and false if not
    */
    bool connectOneToMany(BaseFilter *R, int readerID);
    /**
    * Creates a many to many connection from specific reader to specific writer
    * @param BaseFilter pointer of the filter to be connected
    * @param Integer reader ID
    * @param Integer writer ID
    * @return True if succeeded and false if not
    */
    bool connectManyToMany(BaseFilter *R, int readerID, int writerID);

    /**
    * Disconnects and cleans specified writer
    * @param Integer writer ID
    * @return True if succeeded and false if not
    */
    bool disconnectWriter(int writerId);
    /**
    * Disconnects and cleans specified reader
    * @param Integer reader ID
    * @return True if succeeded and false if not
    */
    bool disconnectReader(int readerId);
    /**
    * Disconnects and cleans all readers and writers
    */
    void disconnectAll();

    /**
    * If it is a master filter a new slave filter is added to the master's list
    * @param Integer slave filter ID
    * @param BaseFilter pointer of the slave filter to be added
    * @return True if succeeded and false if not
    */
    bool addSlave(BaseFilter *slave);

    /**
    * Filter type getter
    * @return filter type
    */
    FilterType getType() {return fType;};
    /**
    * Filter role getter
    * @return filter role
    */
    FilterRole getRole() {return fRole;};

    /**
    * Generates a new and random reader ID
    * @return filter role
    */
    int generateReaderID();
    /**
    * Generates a new and random writer ID
    * @return filter role
    */
    int generateWriterID();
    /**
    * Maximum writers getter
    * @return maximum number of possible writers for this filter
    */
    const unsigned getMaxWriters() const {return maxWriters;};
    /**
    * Maximum readers getter
    * @return maximum number of possible readers for this filter
    */
    const unsigned getMaxReaders() const {return maxReaders;};
    /**
    * Adds a new event to the event queue from this filter
    * @param new event
    */
    virtual void pushEvent(Event e);
    /**
    * Get the state for this filter node
    * @param filter node pointer
    */
    void getState(Jzon::Object &filterNode);

    /**
    * Returns true if filter is enabled or false if not
    * @return Bool enabled
    */
    bool isEnabled(){return enabled;};

    /**
    * Class destructor. Deletes and clears its writers, readers, oframes, dframes and rupdates
    */
    virtual ~BaseFilter();

    /**
    * Sets a dfined wall clock to the filter
    * @param refWallClock reference wall clock
    */
    void setWallClock(std::chrono::system_clock::time_point refWallClock) {wallClock = refWallClock;};
    
    //NOTE: these are public just for testing purposes
    /**
    * Processes frames depending on its role (MASTER or SLAVE)
    * @param integer this integer contains the delay until the method can be executed again
    * @return A vector containing the ids of the filters that can be exectued after 
    * this process (e.g new data has been generated)
    */
    std::vector<int> processFrame(int &ret);
    /**
    * Sets filter frame time
    * @param size_t frame time
    */
    void setFrameTime(std::chrono::nanoseconds fTime);

protected:
    //TODO: all these methods should be described also
    BaseFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool periodic = false);

    std::vector<int> addFrames();
    std::vector<int> removeFrames();
    virtual FrameQueue *allocQueue(int wFId, int rFId, int wId) = 0;

    std::chrono::nanoseconds getFrameTime() {return frameTime;};

    virtual Reader *setReader(int readerID, FrameQueue* queue);
    Reader* getReader(int id);

    bool demandOriginFrames();
    bool demandDestinationFrames();

    bool newEvent();
    void processEvent();
    virtual void doGetState(Jzon::Object &filterNode) = 0;

    bool updateTimestamp();
    std::map<std::string, std::function<bool(Jzon::Node* params)> > eventMap;

    virtual bool runDoProcessFrame() = 0;

    bool removeSlave(int id);
    std::map<int, BaseFilter*> slaves;

protected:
    bool process;

    std::map<int, Reader*> readers;
    std::map<int, Writer*> writers;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    std::map<int, size_t> seqNums;
    FilterType fType;

    float frameTimeMod;
    float bufferStateFrameTimeMod;

    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point lastValidTimestamp;
    std::chrono::nanoseconds duration;
    std::chrono::nanoseconds lastDiffTime;
    std::chrono::nanoseconds diffTime;
    std::chrono::system_clock::time_point wallClock;

    const unsigned maxReaders;
    const unsigned maxWriters;
    std::chrono::nanoseconds frameTime;

private:   
    bool connect(BaseFilter *R, int writerID, int readerID);
    std::vector<int> masterProcessFrame(int& ret);
    std::vector<int> slaveProcessFrame(int& ret);
    std::vector<int> serverProcessFrame(int& ret);
    void processAll();
    bool runningSlaves();
    void execute() {process = true;};
    bool isProcessing() {return process;};
    void updateFrames(std::map<int, Frame*> oFrames_);

private:
    std::priority_queue<Event> eventQueue;
    std::mutex eventQueueMutex;
    std::mutex readersWritersLck;
    
    bool enabled;
    FilterRole const fRole;
    bool force;
};

class OneToOneFilter : public BaseFilter {

protected:
    OneToOneFilter(bool byPassTimestamp, FilterRole fRole_ = MASTER, size_t fTime = 0, bool force_ = false, bool periodic = false);
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool passTimestamp;
    bool runDoProcessFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::addSlave;
    using BaseFilter::setWallClock;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
};

class OneToManyFilter : public BaseFilter {

protected:
    OneToManyFilter(FilterRole fRole_ = MASTER, unsigned writersNum = MAX_WRITERS, size_t fTime = 0, bool force_ = true, bool periodic = false);
    virtual bool doProcessFrame(Frame *org, std::map<int, Frame *> dstFrames) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool runDoProcessFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::addSlave;
    using BaseFilter::setWallClock;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
};

class HeadFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    HeadFilter(FilterRole fRole_ = MASTER, size_t fTime = 0, unsigned writersNum = 1, bool periodic = true);
    virtual bool doProcessFrame(std::map<int, Frame*> dstFrames) = 0;
    
    int getNullWriterID();
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private: 
    bool runDoProcessFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::addSlave;
    using BaseFilter::setWallClock;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
};

class TailFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    TailFilter(FilterRole fRole_ = MASTER, unsigned readersNum = MAX_READERS, size_t fTime = 0, bool periodic = false);
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    FrameQueue *allocQueue(int wFId, int rFId, int wId) {return NULL;};
    bool runDoProcessFrame();
    virtual bool doProcessFrame(std::map<int, Frame*> orgFrames) = 0;
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::setWallClock;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
};

class ManyToOneFilter : public BaseFilter {

protected:
    ManyToOneFilter(FilterRole fRole_ = MASTER, unsigned readersNum = MAX_READERS, size_t fTime = 0, bool force_ = false, bool periodic = false);
    virtual bool doProcessFrame(std::map<int, Frame *> orgFrames, Frame *dst) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:   
    bool runDoProcessFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::addSlave;
    using BaseFilter::setWallClock;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
};

class LiveMediaFilter : public BaseFilter
{
public:
    void pushEvent(Event e);

protected:
    LiveMediaFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS, bool periodic = true);

    virtual bool doProcessFrame() = 0;

private:
    bool runDoProcessFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;
    using BaseFilter::addSlave;
    using BaseFilter::setWallClock;
    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastValidTimestamp;
    using BaseFilter::duration;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    
};

#endif
