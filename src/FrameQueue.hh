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

using namespace std::chrono;

class FrameQueue {

public:
    FrameQueue();
    virtual ~FrameQueue() {};
    virtual Frame *getRear() = 0;
    virtual Frame *getFront() = 0;
    virtual void addFrame() = 0;
    virtual void removeFrame() = 0;
    virtual void flush() = 0;
    virtual Frame *forceGetRear() = 0;
    virtual Frame *forceGetFront(bool &newFrame) = 0;
    virtual bool frameToRead() = 0;
    bool isConnected() {return connected;};
    void setConnected(bool conn) {connected = conn;};

protected:
    virtual bool config() = 0;

    std::atomic<int> rear;
    std::atomic<int> front;
    std::atomic<int> elements;
    std::atomic<bool> connected;
    std::atomic<bool> firstFrame;
    
};

#endif
