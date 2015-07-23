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
    Reader();

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
    Frame* getFrame(int fId, bool force = false);

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
    * Disconnects from its queue (sets queue disconnected) or deletes the queue
    * if it is not connected
    * @return true if successful disconnecting, otherwise returns false
    */
    bool disconnect();
    
    /**
    * Increases the number of filters that make use of this reader
    */
    void addReader();
    
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

protected:
    FrameQueue *queue;

private:
    friend class Writer;
     
    Frame *frame;
    unsigned filters;
    std::map<int, bool> requests;
    unsigned pending;
    std::mutex lck;
};

#endif
