/*
 *  Filter.hh - Filter base classes
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

#include <map>

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#ifndef _IO_INTERFACE_HH
#include "IOInterface.hh"
#endif

#ifndef _WORKER_HH
#include "Worker.hh"
#endif

class MultiIOFilter : Runnable {
    
public:
    virtual void addReader();
    virtual bool removeReader(int id);
    virtual void addWriter();
    virtual bool removeWriter(int id);
    bool connect(int wId, Filter *R, rId);
    bool disconnect(int wId, Filter *R, rId);
    Reader* getReader(id) {return readers[id];};
    
protected:
    MultiIOFilter(int readersNum = 0, int writersNum = 0);
    
    virtual FrameQueue *allocQueue() = 0;
    virtual bool doProcessFrame(std::map<int, Frame *> orgFrames, std::map<int, Frame *> dstFrames) = 0;
    
private:
    bool processFrame();
    bool demandOriginFrames();
    bool demandDestinationFrames();
    void addFrames();
    void removeFrames();
      
private:
    int rwNextId;
    
    std::map<int, bool> rUpdates;
    
    std::map<int, Reader*> readers;
    std::map<int, Reader*> writers;
    std::map<int, Frames*> oFrames;
    std::map<int, Frames*> dFrames;
};

MultiIOFilter::MultiIOFilter(int readersNum, int writersNum)
{
    rwNextId = 0;
    for(int i = 0; i < readersNum; i++, ++rwNextId){
        readers[rwNextId] = new Reader();
        rUpdates[rwNextId] = false;
        oFrames[rwNextId] = NULL;
    }
    for(int i = 0; i < writersNum; i++, ++rwNextId){
        writers[rwNextId] = new Writer();
        dFrames[rwNextId] = NULL;
    }
}

void MultiIOFilter::addReader(){
    ++rwNextId;
    readers[rwNextId] = new Reader();
    rUpdates[rwNextId] = false;
    oFrames[rwNextId] = NULL;
}

bool MultiIOFilter::removeReader(int id){
    if (readers.find(id) != readers.end() && !readers[id].isConnected()){
        readers.erease(id);
        rUpdates.erease(id);
        oFrames.erease(id);
        return true;
    } 
    return false;
}

void MultiIOFilter::addWriter(){
    ++rwNextId;
    writers[rwNextId] = new Writer();
    dFrames[rwNextId] = NULL;
}

bool MultiIOFilter::removeWriter(int id){
    if (writers.find(id) != writers.end() && !writers[id].isConnected()){
        writers.erease(id);
        dFrames.erease(id);
        return true;
    } 
    return false;
}

bool MultiIOFilter::processFrame()
{
    bool newData
    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }
    if (doProcessFrame(orgFrames, dstFrames)){
        addFrames();
    }
    removeFrames();
    return true;
}

bool MultiIOFilter::demandOriginFrames()
{
    for (auto it : readers){
        oFrames[it.first] = it.second->getFrame(true);
        if (oFrames[it.first] == NULL){
            rUpdates[it.first] = false; 
        } else {
            rUpdates[it.first] = true;
        }
    }
}

bool MultiIOFilter::demandDestinationFrames()
{
    for (auto it : writers){
        dFrames[it.first] = it.second->getFrame(true);
    }
}

void MultiIOFilter::addFrames()
{
    for (auto it : writers){
        it.second->addFrame();
    }
}

void MultiIOFilter::removeFrames()
{
    for (auto it : readers){
        if (rUpdates[it.first]){
            it.second->removeFrame();
        }
    }
}

bool MultiIOFilter::connect(int wId, Filter *R, int rId)
{
    if (writers[wId].isConnected()){
        return false;
    }
    Reader *r = R->getReader(rId);
    if (r->isConnected()){
        return false;
    }
    FrameQueue *queue = allocQueue();
    writers[id]->setQueue(queue);
    return writers[id]->connect(r);
}

bool MultiIOFilter::disconnect(int wId, Filter *R, int rId)
{
    if (! writers[wId].isConnected()){
        return false;
    }
    Reader *r = R->getReader(rId);
    if (! r->isConnected()){
        return false;
    }
    return writers[id]->disconnect(r);
}

