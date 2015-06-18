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

#include <atomic>
#include <utility>
#include <map>

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
    bool connect(Reader *reader) const;

    /**
    * Disconnects specific Reader and itself from queue
    * @param pointer to Reader object to disconnect from specific queue
    * @return true if success on disconnecting, otherwise returns false
    */
    bool disconnect(Reader *reader) const;

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
    void addFrame() const;

    /**
    * Disconnects from its queue (sets queue disconnected) or deletes the queue
    * if it is not connected
    * @return true if successful disconnecting, otherwise returns false
    */
    bool disconnect() const;

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
    ~Reader();

    /**
    * Sets reader's pointer to FrameQueue object
    * @param FramQueue object pointer
    */
    void setQueue(FrameQueue *queue);

    /**
    * Checks if has a queue and if it is connected to
    * @return true if connected, otherwise return false
    */
    bool isConnected();

    /**
    * Gets front frame object from queue if possible. If force is set to true and
    * frame is NULL this will flush queue until having a frame object from front
    * @param bool to check if there is a newFrame or not
    * @param bool to force having frame object or not (default set to false)
    * @return Frame object from its queue
    */
    Frame* getFrame(bool &newFrame, bool force = false);

    /**
    * Removes frame element from queue
    */
    void removeFrame();

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

protected:
    mutable FrameQueue *queue;

private:
    friend class Writer;

};

#endif
