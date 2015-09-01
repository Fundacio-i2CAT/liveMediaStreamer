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
#include <memory>

#include "FrameQueue.hh"
#include "IOInterface.hh"
#include "Runnable.hh"
#include "Event.hh"
#include "StreamInfo.hh"

#define DEFAULT_ID 1                /*!< Default ID for unique filter's readers and/or writers. */
#define MAX_WRITERS 16              /*!< Default maximum writers for a filter. */
#define MAX_READERS 16              /*!< Default maximum readers for a filter. */

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
    
    bool shareReader(BaseFilter *shared, int sharedRId, int orgRId);

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
    * @return ID
    */
    unsigned generateReaderID();
    /**
    * Generates a new and random writer ID
    * @return ID
    */
    unsigned generateWriterID();
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

    //NOTE: these are public just for testing purposes
    /**
    * Processes frames
    * @param integer this integer contains the delay until the method can be executed again
    * @return A vector containing the ids of the filters that can be exectued after 
    * this process (e.g new data has been generated)
    */
    std::vector<int> processFrame(int &ret);
    /**
    * Sets filter frame time
    * @param size_t frame time
    */
    void setFrameTime(std::chrono::microseconds fTime);
    
    /**
     * tests if the specfied reader is connected or not
     * @param readerId of the reader to check
     * @return true if, and only if, the reader exists and it is connected.
     */
    bool isRConnected (int rId);
    
    /**
     * tests if the specfied writer is connected or not
     * @param writerId of the writer to check
     * @return true if, and only if, the writer exists and it is connected.
     */
    bool isWConnected (int wId);
    
    /**
     * get the connection data of the specified writer
     * @param writerId of the writer to check
     * @return the connection data. It is a defaut ConnectionData in case of failure (e.g. filter not connected)
     */
    struct ConnectionData getWConnectionData (int wId);

protected:
    BaseFilter(unsigned readersNum = MAX_READERS, unsigned writersNum = MAX_WRITERS, FilterRole fRole_ = REGULAR, bool periodic = false);

    std::vector<int> addFrames(std::map<int, Frame*> &dFrames);
    bool removeFrames(std::vector<int> framesToRemove);
    virtual FrameQueue *allocQueue(struct ConnectionData cData) = 0;

    std::chrono::microseconds getFrameTime() {return frameTime;};

    std::shared_ptr<Reader> setReader(int readerID, FrameQueue* queue);
    std::shared_ptr<Reader> getReader(int id);
    
    //TODO: this should get stream info parameters insted of FrameQueue or get from reader.
    virtual bool specificReaderConfig(int readerID, FrameQueue* queue) = 0;
    virtual bool specificReaderDelete(int readerID) = 0;

    std::vector<int> demandOriginFrames(std::map<int, Frame*> &oFrames);
    std::vector<int> demandOriginFramesBestEffort(std::map<int, Frame*> &oFrames);
    std::vector<int> demandOriginFramesFrameTime(std::map<int, Frame*> &oFrames); 

    bool demandDestinationFrames(std::map<int, Frame*> &dFrames);

    bool newEvent();
    void processEvent();
    virtual void doGetState(Jzon::Object &filterNode) = 0;

    std::map<std::string, std::function<bool(Jzon::Node* params)> > eventMap;

    virtual bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames) = 0;

    void setSyncTs(std::chrono::microseconds ts){syncTs = ts;};
    std::chrono::microseconds getSyncTs(){return syncTs;};

protected:
    std::map<int, std::shared_ptr<Reader>> readers;
    std::map<int, std::shared_ptr<Writer>> writers;
    std::map<int, size_t> seqNums;
    FilterType fType;

    const unsigned maxReaders;
    const unsigned maxWriters;
    std::chrono::microseconds frameTime;

private:
    bool connect(BaseFilter *R, int writerID, int readerID);
    std::vector<int> regularProcessFrame(int& ret);
    std::vector<int> serverProcessFrame(int& ret);
    bool deleteReader(int readerId);
    bool pendingJobs();

private:
    std::priority_queue<Event> eventQueue;
    
    bool enabled;
    FilterRole const fRole;
    std::chrono::microseconds syncTs;
};

class OneToOneFilter : public BaseFilter {

protected:
    OneToOneFilter(FilterRole fRole_= REGULAR, bool periodic = false);
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/);
    
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::readers;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::mtx;
};

class OneToManyFilter : public BaseFilter {

protected:
    OneToManyFilter(unsigned writersNum = MAX_WRITERS, FilterRole fRole_= REGULAR, bool periodic = false);
    virtual bool doProcessFrame(Frame *org, std::map<int, Frame *> &dstFrames) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/);

    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::readers;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;

    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::mtx;
};

class HeadFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    HeadFilter(unsigned writersNum = MAX_WRITERS, FilterRole fRole_ = REGULAR, bool periodic = true);
    virtual bool doProcessFrame(std::map<int, Frame*> &dstFrames) = 0;
    
    int getNullWriterID();
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private: 
    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/);
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};

    using BaseFilter::demandDestinationFrames;
    using BaseFilter::demandOriginFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::readers;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::mtx;
};

class TailFilter : public BaseFilter {
public:
    void pushEvent(Event e);

protected:
    TailFilter(unsigned readersNum = MAX_READERS, FilterRole fRole_ = REGULAR, bool periodic = false);
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:
    FrameQueue *allocQueue(struct ConnectionData cData) {return NULL;};
    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames);
    virtual bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> newFrames) = 0;
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::readers;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::mtx;
};

class ManyToOneFilter : public BaseFilter {

protected:
    ManyToOneFilter(unsigned readersNum = MAX_READERS, FilterRole fRole_ = REGULAR, bool periodic = false);
    virtual bool doProcessFrame(std::map<int, Frame *> &orgFrames, Frame *dst, std::vector<int> newFrames) = 0;
    using BaseFilter::setFrameTime;
    using BaseFilter::getFrameTime;

private:   
    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames);

    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::writers;
    using BaseFilter::readers;
    using BaseFilter::seqNums;
    using BaseFilter::processEvent;
    using BaseFilter::frameTime;
    using BaseFilter::maxReaders;
    using BaseFilter::maxWriters;
    using BaseFilter::mtx;
};

#endif
