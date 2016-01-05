/*
 *  IOInterface.hh - Interface classes of frame processors
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

#ifndef _IO_INTERFACE_HH
#define _IO_INTERFACE_HH

#include <mutex>
#include <utility>
#include <map>
#include <memory>

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

class Reader;

/*! Writer class is an IOInterface dedicated to write frames to an specific queue.
*/
class Writer {

public:
    /**
    * Creates a writer object
    */
    Writer();
    ~Writer();

    /**
    * Connects specific Reader to its queue and sets queue as connected
    * @param pointer to Reader object to connect with through specific queue
    * @return true if success on connecting, otherwise returns false
    */
    bool connect(std::shared_ptr<Reader> reader) const;

    /**
    * Disconnects specific Reader and itself from queue
    * @param pointer to Reader object to disconnect from specific queue
    * @return true if success on disconnecting, otherwise returns false
    */
    bool disconnect(std::shared_ptr<Reader> reader) const;

    /**
    * Check if writer has its queue connected
    * @return true if connected, otherwise returns false
    */
    bool isConnected() const;

    /**
    * Sets writer's queue
    * @param pointer to FrameQueue object
    */
    void setQueue(FrameQueue *queue) const;

    /**
    * Gets rear frame object from queue if possible. If force is set to true and
    * frame is NULL this will flush queue until having a frame object from rear
    * @param bool to force having frame object or not (default set to false)
    * @return Frame object from its queue
    */
    Frame* getFrame(bool force = false) const;

    /**
    * Adds a frame element to its queue
    */
    int addFrame() const;

    /**
    * Disconnects from its queue (sets queue disconnected) or deletes the queue
    * if it is not connected
    * @return true if successful disconnecting, otherwise returns false
    */
    bool disconnect() const;
    
    /**
    * gets the connection data of the associated queue
    * @return the connection data struct of the queue
    */
    struct ConnectionData getCData();

protected:
    mutable FrameQueue *queue;

};

/*! Reader class is an IOInterface dedicated to read frames from an specific queue.
*/
class Reader {
public:
    /**
    * Creates a reader object
    */
    Reader(std::chrono::microseconds wDelay = std::chrono::microseconds(DEFAULT_STATS_TIME_INTERVAL));

    /**
    * Class destructor
    */
    ~Reader();

    /**
    * Checks if has a queue and if it is connected to
    * @return true if connected, otherwise return false
    */
    bool isConnected();

    /**
    * Gets front frame object from queue if possible. If force is set to true and
    * frame is NULL this will flush queue until having a frame object from front
    * @param integer to identify the filter that is actually requesting the frame.
    * @param bool to force having frame object or not (default set to false)
    * @return Frame object from its queue
    */
    Frame* getFrame(int fId, bool &newFrame);

    /**
    * Removes frame element from queue
    */
    int removeFrame(int fId);

    /**
    * Sets queue to connect to
    * @param FrameQueue object pointer to connect to
    */
    void setConnection(FrameQueue *queue);

    /**
    * Get FrameQueue object pointer
    * @return FrameQueue object pointer
    */
    FrameQueue* getQueue() const {return queue;};

    /**
    * Updates connection data and if there isn't anyother filter liked with this 
    * reader it disconnects from its queue (sets queue disconnected) or deletes the queue
    * if it is already disconnected
    * @param int id of the filter requesting the disconnection
    * @return true if successful disconnecting, otherwise returns false
    */
    bool disconnect(int id);
    
    /**
    * Records the filters that are sharing this reader
    * @param int the filter Id that will use this reader.
    * @param int the reader Id that will use the filter.
    */
    void addReader(int fId, int rId);
    
    /**
    * Decreases the number of filters that make use of this reader. It disconnects if filters number is zero.
    * @param int the id of the filter requesting the removal
    */
    void removeReader(int id);

    /**
    * Get FrameQueue elements number
    * @return FrameQueue elements number
    */
    size_t getQueueElements();

    /**
    * Get average frame delay
    * @return average frame delay in microseconds
    */
    std::chrono::microseconds getAvgDelay();

    /**
    * Get lost blocs
    * @return lost blocs in size_t
    */
    size_t getLostBlocs();

protected:
    FrameQueue *queue;

private:
    bool disconnectQueue();
    void measureDelay();
    bool allRead();

    friend class Writer;
     
    Frame *frame;
    //Fist boolean is related to getFrame status, second one to remove frame status
    std::map<int, std::pair<bool, bool>> filters;
    bool ready;
    
    std::mutex lck;

    //Stats
    std::chrono::microseconds avgDelay;
    std::chrono::microseconds delay;
    std::chrono::microseconds windowDelay;
    std::chrono::microseconds lastTs;
    std::chrono::microseconds timeCounter;
    size_t frameCounter;
};

#endif
