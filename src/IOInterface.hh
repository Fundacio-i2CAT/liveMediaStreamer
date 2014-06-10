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
    bool connect(Reader *reader);
    void disconnect(Reader *reader);
    bool isConnected() {return connected;};
    void setQueue(FrameQueue *queue);
    Frame* getFrame(bool force = false);
    void addFrame();

protected:
    FrameQueue *queue;

private:
    bool connected;
};


class Reader {
public:
    Reader();
    ~Reader();
    void setQueue(FrameQueue *queue);
    bool isConnected() {return connected;};
    Frame* getFrame(bool force = false);
    void removeFrame();
    void setConnection(FrameQueue *queue);
    const FrameQueue* getQueue() const {return queue;};

protected:
    FrameQueue *queue;

private:
    friend class Writer;

    void disconnect();
    
    bool connected;
};

#endif
