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
 *			  Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _FILTER_HH
#define _FILTER_HH

#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <liveMedia/liveMedia.hh>
#include <BasicUsageEnvironment.hh>

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#ifndef _IO_INTERFACE_HH
#include "IOInterface.hh"
#endif

#ifndef _WORKER_HH
#include "Worker.hh"
#endif

#include "Event.hh"

#define DEFAULT_ID 1
#define MAX_WRITERS 16
#define MAX_READERS 16
#define VIDEO_DEFAULT_FRAMERATE 25 //fps
#define RETRY 500 //us

class MasterFilter;
class SlaveFilter;

class BaseFilter : public Runnable {

public:
    bool connectOneToOne(BaseFilter *R, bool slaveQueue = false);
    bool connectManyToOne(BaseFilter *R, int writerID, bool slaveQueue = false);
    bool connectOneToMany(BaseFilter *R, int readerID, bool slaveQueue = false);
    bool connectManyToMany(BaseFilter *R, int readerID, int writerID, bool slaveQueue = false);

    //bool disconnect(BaseFilter *R, int writerID, int readerID);
    bool disconnectWriter(int writerId);
    bool disconnectReader(int readerId);
    void disconnectAll();

    FilterType getType() {return fType;};
    FilterRole getRole() {return fRole;};
    int generateReaderID();
    int generateWriterID();
    const unsigned getMaxWriters() const {return maxWriters;};
    const unsigned getMaxReaders() const {return maxReaders;};
    virtual void pushEvent(Event e);
    void getState(Jzon::Object &filterNode);
    //bool deleteReader(int id);
    int getWorkerId(){return workerId;};
    void setWorkerId(int id){workerId = id;};
    bool isEnabled(){return enabled;};
    virtual ~BaseFilter();

    //NOTE: these are public just for testing purposes
    size_t processFrame();
    void setFrameTime(size_t fTime);

protected:
    BaseFilter();

    void addFrames();
    void removeFrames();
    bool hasFrames();
    virtual FrameQueue *allocQueue(int wId) = 0;

    std::chrono::microseconds getFrameTime() {return frameTime;};

    virtual size_t slaveProcessFrame() = 0;
    virtual size_t masterProcessFrame() = 0;

    virtual Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false);
    Reader* getReader(int id);

    bool demandOriginFrames();
    bool demandDestinationFrames();

    bool newEvent();
    void processEvent();
    virtual void doGetState(Jzon::Object &filterNode) = 0;

    void updateTimestamp();
    std::map<std::string, std::function<void(Jzon::Node* params, Jzon::Object &outputNode)> > eventMap;

protected:
    std::map<int, Reader*> readers;
    std::map<int, const Writer*> writers;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    FilterType fType;

    float frameTimeMod;
    float bufferStateFrameTimeMod;

    std::chrono::microseconds frameTime;
    std::chrono::microseconds timestamp;
    std::chrono::microseconds lastDiffTime;
    std::chrono::microseconds diffTime;
    std::chrono::microseconds wallClock;

    unsigned maxReaders;
    unsigned maxWriters;
    FilterRole fRole;
    bool force;
    bool sharedFrames;

private:
    bool connect(BaseFilter *R, int writerID, int readerID, bool slaveQueue = false);

private:
    std::priority_queue<Event> eventQueue;
    std::mutex eventQueueMutex;
    int workerId;
    bool enabled;
    std::map<int, bool> rUpdates;
};

class MasterFilter : virtual public BaseFilter {

public:
    MasterFilter();
    bool addSlave(int id, SlaveFilter *slave);

protected:
    bool removeSlave(int id);

    virtual bool runDoProcessFrame() = 0;

    std::map<int, SlaveFilter*> slaves;

private:
    size_t masterProcessFrame();
    void processAll();
    bool runningSlaves();
};


class SlaveFilter : virtual public BaseFilter {

public:
    SlaveFilter();

protected:

    virtual bool runDoProcessFrame() = 0;

private:
    friend class MasterFilter;
    
    void setSharedFrames(bool sharedFrames_) {sharedFrames = sharedFrames_;};
    size_t slaveProcessFrame();
    void execute() {process = true;};
    bool isProcessing() {return process;};
    void updateFrames(std::map<int, Frame*> oFrames_);
    
    bool process;
};

class OneToOneFilter : public MasterFilter, public SlaveFilter {

protected:
    OneToOneFilter(size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
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
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::fRole;
    using BaseFilter::force;

    using MasterFilter::sharedFrames;

    void stop() {};
};

class OneToManyFilter : public MasterFilter, public SlaveFilter {

protected:
    OneToManyFilter(unsigned writersNum = MAX_WRITERS, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
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
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::fRole;
    using BaseFilter::force;

    void stop() {};
};

class HeadFilter : public MasterFilter, public SlaveFilter {
public:
    //TODO:implement this function
    void pushEvent(Event e);

protected:
    HeadFilter(unsigned writersNum = MAX_WRITERS, size_t fTime = 0, FilterRole fRole_ = MASTER);
    int getNullWriterID();
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::fRole;
    using BaseFilter::force;
};

class TailFilter : public MasterFilter, public SlaveFilter {
public:
    void pushEvent(Event e);

protected:
    TailFilter(unsigned readersNum = MAX_READERS, size_t fTime = 0, FilterRole fRole_ = MASTER, bool sharedFrames_ = true);
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    FrameQueue *allocQueue(int wId) {return NULL;};
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::fRole;
    using BaseFilter::force;
};

class ManyToOneFilter : public MasterFilter, public SlaveFilter {

protected:
    ManyToOneFilter(unsigned readersNum = MAX_READERS, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
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
    using BaseFilter::processEvent;
    using BaseFilter::updateTimestamp;

    using BaseFilter::frameTime;
    using BaseFilter::timestamp;
    using BaseFilter::lastDiffTime;
    using BaseFilter::diffTime;
    using BaseFilter::wallClock;

    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::fRole;
    using BaseFilter::force;

    void stop() {};
};

class LiveMediaFilter : public BaseFilter
{
public:
    void pushEvent(Event e);
    UsageEnvironment* envir() {return env;}

protected:
    LiveMediaFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS, size_t fTime = 0, FilterRole fRole_ = NETWORK);
    size_t processFrame();

    UsageEnvironment* env;
    uint8_t watch;

private:

    //TODO decide whether roleProcessFrame methods are basefilter pure vurtual
    //methods or should be only defined at its respective role class
    size_t slaveProcessFrame() {return 0;};
    size_t masterProcessFrame() {return 0;};

};

#endif
