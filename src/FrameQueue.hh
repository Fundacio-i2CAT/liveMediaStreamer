/*
 *  FrameQueue.hh - A FIFO queue for frames
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors: David Cassany <david.cassany@i2cat.net>
 *           Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _FRAME_QUEUE_HH
#define _FRAME_QUEUE_HH

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#include <sys/time.h>
#include <chrono>
#include <list>
#include <vector>
#include "Types.hh"
#include "StreamInfo.hh"
#include "Utils.hh"

#define FULL_THRESHOLD 0.9

/*! FrameQueue class is pure abstract class that represents buffering structure
    of the pipeline
*/

/**
 * A structure to represent consumers connection identifiers. The reader ID of the consumer filter and consumer filter ID. 
 * By default all values are set to -1, which is an invalid id.
 */

struct ReaderData
{
    int rFilterId = -1;
    int readerId = -1;
};

/**
 * A structure to represent connection identifiers. The writer ID of the producer filter, producer filter ID, 
 * and an array of the consumers data structs. By default all values are set to -1, which is an invalid id.
 */

struct ConnectionData
{
    int wFilterId = -1;
    int writerId = -1;
    std::list<ReaderData> readers;
};



class FrameQueue {

public:
    /**
    * Class constructor
    * @param connectionData struct that contains reader and writer filters identifiers
    * @param si description of the stream passing through this queue
    */
    FrameQueue(ConnectionData cData, const StreamInfo *si = NULL) :
            rear(0), front(0), connected(false), firstFrame(false),
            lostBlocs(0), connectionData(cData), streamInfo(si) {};

    /**
    * Class destructor
    */
    virtual ~FrameQueue() {};

    /**
    * Returns frame object from queue's rear
    * @return frame object
    */
    virtual Frame *getRear() = 0;

    /**
    * Returns frame object from queue's front
    * @return frame object
    */
    virtual Frame *getFront() = 0;

    /**
    * Adds frame to queue elements
    * @return the ids of the reader filters that has a new frame available.
    */
    virtual std::vector<int> addFrame() = 0;

    /**
    * Removes frame from queue elements
    * @return the id of the writer filter that introduced the frame consumed.
    */
    virtual int removeFrame() = 0;

    /**
    * Counts lost blocs and flushes the queue
    */
    void flush() 
    { 
        lostBlocs++;
        doFlush();
    };
    
    /**
    * Flushes the queue
    */
    virtual void doFlush() = 0;
    
    /**
    * Get lost blocs
    */
    size_t getLostBlocs() { return lostBlocs; };

    /**
    * Forces getting frame from queue's rear
    * @return frame object
    */
    virtual Frame *forceGetRear() = 0;

    /**
    * Forces getting frame from queue's front
    * @return frame object
    */
    virtual Frame *forceGetFront() = 0;

    /**
    * To know if the queue is connected with a reader and a writer
    * @return true if connected and false if not
    */
    bool isConnected() {return connected;};

    /**
    * Sets the queue as connected when it has a reader and a writer
    * @param boolean to set connected to true or false
    */
    void setConnected(bool conn) {connected = conn;};
    
    /**
    * Get number of elements in the queue
    * @return elements
    */
    virtual unsigned getElements() const = 0;
    
    /**
    * Tests if the current queue is full or not
    * @return true if the number of elements exceeds the threshold level
    */
    virtual bool isFull() const = 0;
    
    /**
    * Gets the connection cData.
    * @return the struct that contains the connection data.
    */
    ConnectionData getCData() const {return connectionData;};
    
    /**
    * Adds reader to the CData
    * @param int consumer filter id
    * @param int reader id
    * @return true if the addition succeeded
    */
    bool addReaderCData(int rFilterId, int readerId) {
        for(auto r : connectionData.readers){
            if (r.rFilterId == rFilterId){
                return false;
            }
        }
        
        ReaderData reader;
        
        reader.rFilterId = rFilterId;
        reader.readerId = readerId;
        
        connectionData.readers.push_back(reader);
        
        return true;
    };
    
    /**
    * Removes reader to the CData
    * @param int consumer filter id
    * @return true if the removal succeeded
    */
    bool removeReaderCData(int fId) {            
        std::list<ReaderData>::iterator i = connectionData.readers.begin();
        while (i != connectionData.readers.end())
        {
            if ((*i).rFilterId == fId){
                i = connectionData.readers.erase(i);
                return true;
            } else {
                ++i;
            }
        }
        
        return false;
    };

    /**
    * Gets the StreamInfo for the stream passing through this queue.
    * @return the struct that contains the connection data.
    */
    const StreamInfo *getStreamInfo() const {return streamInfo;};

protected:
    size_t rear;
    size_t front;
    bool connected;
    bool firstFrame;
    size_t lostBlocs;

    ConnectionData connectionData;

    const StreamInfo *streamInfo;

};

#endif
