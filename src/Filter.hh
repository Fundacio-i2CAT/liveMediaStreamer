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
 *
 */

#ifndef _FILTER_HH
#define _FILTER_HH

#include <map>
#include <vector>
#include <queue>
#include <mutex>

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#ifndef _IO_INTERFACE_HH
#include "IOInterface.hh"
#endif

#ifndef _WORKER_HH
#include "Worker.hh"
#endif

#include <iostream>

#include "Event.hh"
#define DEFAULT_ID 1
#define MAX_WRITERS 16
#define MAX_READERS 16
#define VIDEO_DEFAULT_FRAMERATE 25 //fps

class MasterFilter;
class SlaveFilter;


class BaseFilter : public Runnable {

public:
    bool connectOneToOne(BaseFilter *R, bool slaveQueue = false);
    bool connectManyToOne(BaseFilter *R, int writerID, bool slaveQueue = false);
    bool connectOneToMany(BaseFilter *R, int readerID, bool slaveQueue = false);
    bool connectManyToMany(BaseFilter *R, int readerID, int writerID, bool slaveQueue = false);

    bool disconnect(BaseFilter *R, int writerID, int readerID);
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
    bool deleteReader(int id);
    int getWorkerId(){return workerId;};
    void setWorkerId(int id){workerId = id;};
    bool isEnabled(){return enabled;};
    virtual ~BaseFilter();

protected:
    BaseFilter(unsigned maxReaders_, unsigned maxWriters_, FilterRole fRole_ = MASTER, bool force_ = false);
	void removeFrames();
    bool hasFrames();
    virtual FrameQueue *allocQueue(int wId) = 0;
    virtual bool processFrame(bool removeFrame = true) = 0;
    virtual Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false);
    virtual void doGetState(Jzon::Object &filterNode) = 0;

    Reader* getReader(int id);
    bool demandOriginFrames();
    bool demandDestinationFrames();
    void addFrames();
    void processEvent();
    bool newEvent();

    std::map<std::string, std::function<void(Jzon::Node* params, Jzon::Object &outputNode)> > eventMap;

protected:
    std::map<int, Reader*> readers;
    std::map<int, Writer*> writers;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    FilterType fType;

    float frameTimeMod;
    float bufferStateFrameTimeMod;
    void updateTimestamp();

    std::chrono::microseconds frameTime;
    std::chrono::microseconds timestamp;
    std::chrono::microseconds lastDiffTime;
    std::chrono::microseconds diffTime;
    std::chrono::microseconds getFrameTime();

private:
    bool connect(BaseFilter *R, int writerID, int readerID, bool slaveQueue = false);

    FilterRole fRole;
    unsigned maxReaders;
    unsigned maxWriters;
    bool force;
    std::priority_queue<Event> eventQueue;
    std::mutex eventQueueMutex;
    int workerId;
    bool enabled;

    std::map<int, bool> rUpdates;
};

class MasterFilter : virtual public BaseFilter {

public:
    MasterFilter(unsigned maxReaders_, unsigned maxWriters_, FilterRole fRole_, bool force_);

protected:
    bool addSlave(int id, SlaveFilter *slave);
    bool removeSlave(int id);

    bool allFinished();
    void processAll();

    std::map<int, SlaveFilter*> slaves;
};


class SlaveFilter : virtual public BaseFilter {

public:
    SlaveFilter(unsigned maxReaders_, unsigned maxWriters_, FilterRole fRole_, bool force_);

protected:
    bool getFinished(){return finished;};
    void setFalse();

    void process();
    std::atomic<bool> finished;

};


class OneToOneFilter : public MasterFilter, public SlaveFilter {

protected:
    OneToOneFilter(FilterRole fRole_ = MASTER, bool force_ = false);
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;

private:
    bool processFrame(bool removeFrame = true);
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    //using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    void stop() {};
};

class OneToManyFilter : public MasterFilter, public SlaveFilter {

protected:
    OneToManyFilter(unsigned writersNum = MAX_WRITERS, FilterRole fRole_ = MASTER, bool force_ = false);
    virtual bool doProcessFrame(Frame *org, std::map<int, Frame *> dstFrames) = 0;

private:
    bool processFrame(bool removeFrame = true);
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    void stop() {};
};

class HeadFilter : public MasterFilter, public SlaveFilter {
public:
    //TODO:implement this function
    void pushEvent(Event e);

protected:
    HeadFilter(unsigned writersNum = MAX_WRITERS, FilterRole fRole_ = MASTER);
    int getNullWriterID();

private:
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
};

class TailFilter : public MasterFilter, public SlaveFilter {
public:
    void pushEvent(Event e);

protected:
    TailFilter(unsigned readersNum = MAX_READERS, FilterRole fRole_ = MASTER);

private:
    FrameQueue *allocQueue(int wId) {return NULL;};
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
};

class ManyToOneFilter : public MasterFilter, public SlaveFilter {

protected:
    ManyToOneFilter(unsigned readersNum = MAX_READERS, FilterRole fRole_ = MASTER, bool force_ = false);
    virtual bool doProcessFrame(std::map<int, Frame *> orgFrames, Frame *dst) = 0;

private:
    bool processFrame(bool removeFrame = true);
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
    void stop() {};
};

#endif
