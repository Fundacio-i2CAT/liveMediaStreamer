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

#include <atomic>
#include <sys/time.h>
#include <chrono>
#include "Types.hh"

#define MAX_FRAMES 2000000
#define SLOW_THRESHOLD 0.4
#define FAST_THRESHOLD 0.6

using namespace std::chrono;

/*! FrameQueue class is pure abstract class that represents buffering structure
    of the pipeline
*/
class FrameQueue {

public:
    /**
    * Creates a frame object
    */
    FrameQueue() : rear(0), front(0), elements(0), connected(false), firstFrame(false) {};
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
    */
    virtual void addFrame() = 0;

    /**
    * Removes frame from queue elements
    */
    virtual void removeFrame() = 0;

    /**
    * Flushes the queue
    */
    virtual void flush() = 0;

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
    * To know if there is a new frame to read
    * @return true if there is a new frame or false if not
    */
    virtual bool frameToRead() = 0;

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
    unsigned getElements() {return elements;};

protected:
    std::atomic<unsigned> rear;
    std::atomic<unsigned> front;
    std::atomic<unsigned> elements;
    std::atomic<bool> connected;
    std::atomic<bool> firstFrame;
    
};

#endif
