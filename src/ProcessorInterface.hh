/*
 *  ProcessorInterface.hh - Interface classes of frame processors
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
 *            
 */

#include <atomic>

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

class Reader;
class Writer;

class ProcessorInterface {
    
protected:
    ProcessorInterface(ProcessorInterface *otherSide_ = NULL);
    virtual ~ProcessorInterface() {};
   
public:
    static void connect(ProcessorInterface *, ProcessorInterface *);
    static void disconnect(ProcessorInterface *, ProcessorInterface *);
    bool isConnected();
    bool isConnected(ProcessorInterface *);
    ProcessorInterface *getConnectedTo() const;
    ProcessorInterface *getOtherSide() const;
    void setOtherSide(ProcessorInterface *otherSide_);
    FrameQueue *frameQueue() const;
    
protected:
    ProcessorInterface *connectedTo;
    ProcessorInterface *otherSide;
    FrameQueue *queue;
};

class Reader : public ProcessorInterface {
    
public:
    void receiveFrame();
    void setQueue(FrameQueue *queue);
    
protected:
    Reader(Writer *otherSide_ = NULL);
    virtual void toProcess() {};
       
};

class Writer : public ProcessorInterface {
    
protected:
    Writer(Reader *otherSide_ = NULL);
    
    bool isQueueConnected();
    void supplyFrame();
    virtual FrameQueue* allocQueue() = 0;
    bool connectQueue();    
};

class ReaderWriter : public Reader, public Writer {  

public:
    void processFrame();

protected:
    ReaderWriter();
    
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;

private:
    //TODO these might have to be protected virtual
    Frame *org, *dst;
};

