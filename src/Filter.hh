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
#define RETRY 500                   /*!< Default retry time in microseconds (ms). */

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
    bool addSlave(int id, BaseFilter *slave);

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
    * Returns its worker ID
    * @return Integer worker ID
    */
    int getWorkerId(){return workerId;};
    /**
    * Sets the filter's worker ID
    * @param Integer worker ID
    */
    void setWorkerId(int id){workerId = id;};
    /**
    * Returns true if filter is enabled or false if not
    * @return Bool enabled
    */
    bool isEnabled(){return enabled;};

    /**
    * Class destructor. Deletes and clears its writers, readers, oframes, dframes and rupdates
    */
    virtual ~BaseFilter();

    //NOTE: these are public just for testing purposes
    /**
    * Processes frames as a function of its role
    * @return time to wait until next frame should be processed in microseconds.
    */
    std::chrono::microseconds processFrame();
    /**
    * Sets filter frame time
    * @param size_t frame time
    */
    void setFrameTime(std::chrono::microseconds fTime);

protected:
    BaseFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS, FilterRole fRole_ = MASTER, bool sharedFrames_ = true);

    void addFrames();
    void removeFrames();
    virtual FrameQueue *allocQueue(int wId) = 0;

    std::chrono::microseconds getFrameTime() {return frameTime;};

    virtual Reader *setReader(int readerID, FrameQueue* queue);
    Reader* getReader(int id);

    bool demandOriginFrames(std::chrono::microseconds &outTimestamp);
    bool demandOriginFramesBestEffort(std::chrono::microseconds &outTimestamp);
    bool demandOriginFramesFrameTime(std::chrono::microseconds &outTimestamp); 

    bool demandDestinationFrames();

    bool newEvent();
    void processEvent();
    virtual void doGetState(Jzon::Object &filterNode) = 0;

    std::map<std::string, std::function<bool(Jzon::Node* params)> > eventMap;

    virtual bool runDoProcessFrame(std::chrono::microseconds outTimestamp) = 0;

    bool removeSlave(int id);
    std::map<int, BaseFilter*> slaves;

protected:
    bool process;

    std::map<int, Reader*> readers;
    std::map<int, const Writer*> writers;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    std::map<int, size_t> seqNums;
    FilterType fType;

    unsigned maxReaders;
    unsigned maxWriters;
    std::chrono::microseconds frameTime;

private:
    bool connect(BaseFilter *R, int writerID, int readerID);
    std::chrono::microseconds masterProcessFrame();
    void processAll();
    bool runningSlaves();
    void setSharedFrames(bool sharedFrames_);
    std::chrono::microseconds slaveProcessFrame();
    void execute() {process = true;};
    bool isProcessing() {return process;};
    void updateFrames(std::map<int, Frame*> oFrames_);

private:
    std::priority_queue<Event> eventQueue;
    std::mutex eventQueueMutex;
    int workerId;
    bool enabled;
    std::map<int, bool> rUpdates;
    FilterRole const fRole;
    bool sharedFrames;
    std::chrono::microseconds syncTs;
};

class OneToOneFilter : public BaseFilter {

protected:
    OneToOneFilter(FilterRole fRole_ = MASTER, bool sharedFrames_ = true);
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool runDoProcessFrame(std::chrono::microseconds outTimestamp);
    
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::addSlave;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;

    void stop() {};
};

class OneToManyFilter : public BaseFilter {

protected:
    OneToManyFilter(FilterRole fRole_ = MASTER, bool sharedFrames_ = true, unsigned writersNum = MAX_WRITERS);
    virtual bool doProcessFrame(Frame *org, std::map<int, Frame *> dstFrames) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool runDoProcessFrame(std::chrono::microseconds outTimestamp);

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
    using BaseFilter::addSlave;

    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;

    void stop() {};
};

class HeadFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    HeadFilter(FilterRole fRole_ = MASTER, unsigned writersNum = 1);
    virtual bool doProcessFrame(std::map<int, Frame*> dstFrames) = 0;
    
    int getNullWriterID();
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private: 
    bool runDoProcessFrame(std::chrono::microseconds outTimestamp);

    using BaseFilter::demandDestinationFrames;
    using BaseFilter::demandOriginFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::addSlave;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    
    void stop() {};
};

class TailFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    TailFilter(FilterRole fRole_ = MASTER, bool sharedFrames_ = true, unsigned readersNum = MAX_READERS);
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    FrameQueue *allocQueue(int wId) {return NULL;};
    bool runDoProcessFrame(std::chrono::microseconds outTimestamp);
    virtual bool doProcessFrame(std::map<int, Frame*> orgFrames) = 0;

    using BaseFilter::demandDestinationFrames;
    using BaseFilter::demandOriginFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;

    void stop() {};
};

class ManyToOneFilter : public BaseFilter {

protected:
    ManyToOneFilter(FilterRole fRole_ = MASTER, bool sharedFrames_ = true, unsigned readersNum = MAX_READERS);
    virtual bool doProcessFrame(std::map<int, Frame *> orgFrames, Frame *dst) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:   
    bool runDoProcessFrame(std::chrono::microseconds outTimestamp);

    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::addSlave;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;

    void stop() {};
};

class LiveMediaFilter : public BaseFilter
{
public:
    void pushEvent(Event e);

protected:
    LiveMediaFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS);

    virtual bool runDoProcessFrame(std::chrono::microseconds outTimestamp) = 0;

private:
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::addSlave;
    using BaseFilter::frameTime;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    
};

#endif
