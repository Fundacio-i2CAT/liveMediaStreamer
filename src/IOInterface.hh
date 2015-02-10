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

class Writer {

public:
    Writer();
    ~Writer();
    bool connect(Reader *reader) const;
    bool disconnect(Reader *reader) const;
    bool isConnected() const;
    void setQueue(FrameQueue *queue) const;
    Frame* getFrame(bool force = false) const;
    void addFrame() const;
	FrameQueue* getQueue() const {return queue;};
    bool disconnect() const;

protected:
    mutable FrameQueue *queue;

};


class Reader {
public:
    Reader(bool sharedQueue = false);
    ~Reader();
    void setQueue(FrameQueue *queue);
    bool isConnected();
    Frame* getFrame(QueueState &state, bool &newFrame, bool force = false);
    void removeFrame();
    void setConnection(FrameQueue *queue);
    FrameQueue* getQueue() const {return queue;};
    bool disconnect();

protected:
    mutable FrameQueue *queue;

private:
    friend class Writer;

    bool sharedQueue;
};

#endif
