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

#ifndef _PROCESSOR_INTERFACE_HH
#define _PROCESSOR_INTERFACE_HH

#include <atomic>
#include <utility> 
#include <map>

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

class MultiReader;
class MultiWriter;

class SingleReader;
class SingleWriter;

class ProcessorInterface {
    
protected:
    ProcessorInterface(ProcessorInterface *otherSide_ = NULL);
    virtual ~ProcessorInterface() {};
   
public:
    static void connect(MultiReader *R, MultiWriter *W);
    static void disconnect(ProcessorInterface *, ProcessorInterface *);
    bool isConnected();
    bool isConnectedById(int id);
    int getId() {return id;};
    std::map<int, std::pair<ProcessorInterface*, FrameQueue*>> getConnectedTo() {return connectedTo;};
    
    ProcessorInterface *getOtherSide() {return otherSide;};
    
    void setOtherSide(ProcessorInterface *otherSide_);
    
    static int currentId;
    
protected:
    ProcessorInterface *otherSide;
    std::map<int, std::pair<ProcessorInterface*, FrameQueue*>> connectedTo;
    
private:
    int id;
};

class MultiReader : public ProcessorInterface {
    
public:
    virtual bool exceptMultiI(){ return true;};
    virtual void newReaderConnection() = 0;
    
protected:
    friend class MultiWriter;
    
    MultiReader(MultiWriter *otherSide_ = NULL);
    void setQueueById(int id, FrameQueue *queue);
    virtual bool demandFrames() = 0;
};

class SingleReader : public MultiReader {

public:
    bool exceptMultiI(){ return false;};
    FrameQueue *frameQueue();
    
protected:
    friend class SingleWriter;
    
    void setQueue(FrameQueue *queue);
    SingleReader(SingleWriter *otherSide_ = NULL);
    virtual bool demandFrame() = 0;
    
private:
    bool demandFrames() {};
    void newReaderConnection() {};
};

class MultiWriter : public ProcessorInterface {
    
public:
    virtual bool exceptMultiO(){ return true;};
    virtual void newWriterConnection() = 0;
    
protected:
    friend class ProcessorInterface;
    
    MultiWriter(MultiReader *otherSide_ = NULL);
    
    bool isQueueConnectedById(int id);
    virtual void supplyFrames(bool newFrame) = 0;
    virtual FrameQueue* allocQueue() = 0;
    bool connectQueueById(int id);
};

class SingleWriter : public MultiWriter {
    
public:
    bool exceptMultiO(){ return false;};
    FrameQueue *frameQueue();
    
protected:
    SingleWriter(SingleReader *otherSide_ = NULL);
    
    bool isQueueConnected();
    virtual void supplyFrame(bool newFrame) = 0;
    bool connectQueue();
    
private:
    void supplyFrames(bool newFrame){};
    void newWriterConnection() {};
};



#endif
