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

#include "Filter.hh"
#include "Utils.hh"
#include <iostream>
#include <thread>
#include <chrono>

#define RETRIES 8
#define TIMEOUT 2500 //us

BaseFilter::BaseFilter(int maxReaders_, int maxWriters_, bool force_) : 
    force(force_), maxReaders(maxReaders_), maxWriters(maxWriters_)
{

}

Reader* BaseFilter::getReader(int id) 
{
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}

Reader* BaseFilter::setReader(int readerID, FrameQueue* queue)
{
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    return r;
}

int BaseFilter::generateReaderID()
{
    if (maxReaders == 1) {
        return DEFAULT_ID;
    }

    int id = rand();

    while (readers.count(id) > 0) {
        id = rand();
    }

    return id;
}

int BaseFilter::generateWriterID()
{
    if (maxWriters == 1) {
        return DEFAULT_ID;
    }

    int id = rand();

    while (writers.count(id) > 0) {
        id = rand();
    }

    return id;
}

bool BaseFilter::demandOriginFrames()
{
    bool newFrame = false;
    bool missedOne = false;
    for(int i = 0; i < RETRIES; i++){
        for (auto it : readers) {
            if (!it.second->isConnected()) {
                readers.erase(it.first);
                continue;
            }

            oFrames[it.first] = it.second->getFrame();
            if (oFrames[it.first] == NULL) {
                oFrames[it.first] = it.second->getFrame(force);
                rUpdates[it.first] = false;
                missedOne = true;
            } else {
                rUpdates[it.first] = true;
                newFrame = true;
            }
        }

        if (!force && missedOne && newFrame){
            std::this_thread::sleep_for(std::chrono::microseconds(TIMEOUT));
        } else {
            break;
        }
    }

    return newFrame;
}

bool BaseFilter::demandDestinationFrames()
{
    bool newFrame = false;
    for (auto it : writers){
        if (it.second->isConnected()){
            dFrames[it.first] = it.second->getFrame(true);
            newFrame = true;
        }
    }
    return newFrame;
}

void BaseFilter::addFrames()
{
    for (auto it : writers){
        if (it.second->isConnected()){
            it.second->addFrame();
        }
    }
}

void BaseFilter::removeFrames()
{
    for (auto it : readers){
        if (rUpdates[it.first]){
            it.second->removeFrame();
        }
    }
}

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID, bool slaveQueue) 
{
    Reader* r;
    FrameQueue *queue;
    
    utils::debugMsg("slaveQueue Value: " + std::to_string(slaveQueue));
    
    if (writers.size() < getMaxWriters() && writers.count(writerID) <= 0) {
        writers[writerID] = new Writer();
        utils::debugMsg("New writer created " + std::to_string(writerID));
    }
    
	if (slaveQueue) {
		if (writers.count(writerID) > 0 && !writers[writerID]->isConnected()) {
            utils::errorMsg("Writer " + std::to_string(writerID) + " null or not connected");
		    return false;
		}
	} else {
		if (writers.count(writerID) > 0 && writers[writerID]->isConnected()) {
            utils::errorMsg("Writer " + std::to_string(writerID) + " null or already connected");
		    return false;
		}
	}
    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        return false;
    }
	if (slaveQueue) {
		queue = writers[writerID]->getQueue();
	} else {
    	queue = allocQueue(writerID);
        utils::debugMsg("New queue allocated for writer " + std::to_string(writerID));
	}
    
    if (!(r = R->setReader(readerID, queue))) {
        utils::errorMsg("Could not set the queue to the reader");
        return false;
    }
	
	if (!slaveQueue) {
    	writers[writerID]->setQueue(queue);
	}
    return writers[writerID]->connect(r);
}

bool BaseFilter::connectOneToOne(BaseFilter *R, bool slaveQueue)
{
    int writerID = R->generateWriterID();
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectManyToOne(BaseFilter *R, int writerID, bool slaveQueue)
{
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectManyToMany(BaseFilter *R, int readerID, int writerID, bool slaveQueue)
{
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectOneToMany(BaseFilter *R, int readerID, bool slaveQueue)
{
    int writerID = R->generateWriterID();
    return connect(R, writerID, readerID, slaveQueue);
}


bool BaseFilter::connect(Reader *r)
{
    if (writers.size() < getMaxWriters() && writers.count(DEFAULT_ID) <= 0) {
        writers[DEFAULT_ID] = new Writer();
    }
    
    if (writers[DEFAULT_ID]->isConnected()) {
        return false;
    }

    if (r->isConnected()){
        return false;
    }
    
    FrameQueue *queue = allocQueue(DEFAULT_ID);
    writers[DEFAULT_ID]->setQueue(queue);
    return writers[DEFAULT_ID]->connect(r);
}


bool BaseFilter::disconnect(int wId, BaseFilter *R, int rId)
{
    if (!writers[wId]->isConnected()){
        return false;
    }

    Reader *r = R->getReader(rId);
    if (!r->isConnected()){
        return false;
    }
    dFrames.erase(wId);
    R->oFrames.erase(rId);
    writers[wId]->disconnect(r);
    writers.erase(wId);
    return true;
}

void BaseFilter::processEvent() 
{
    eventQueueMutex.lock();

    while(newEvent()) {

        Event e = eventQueue.top();
        std::string action = e.getAction();
        Jzon::Node* params = e.getParams();
        Jzon::Object outputNode;


        if (action.empty()) {
            break;
        }

        if (eventMap.count(action) <= 0) {
            break;
        }
        
        eventMap[action](params, outputNode);
        e.sendAndClose(outputNode);

        eventQueue.pop();
    }

    eventQueueMutex.unlock();
}

bool BaseFilter::newEvent() 
{
    if (eventQueue.empty()) {
        return false;
    }

    Event tmp = eventQueue.top();
    if (!tmp.canBeExecuted(std::chrono::system_clock::now())) {
        return false;
    }

    return true;
}

void BaseFilter::pushEvent(Event e)
{
    eventQueueMutex.lock();
    eventQueue.push(e);
    eventQueueMutex.unlock();
}

void BaseFilter::getState(Jzon::Object &filterNode)
{
    eventQueueMutex.lock();
    filterNode.Add("type", utils::getFilterTypeAsString(fType));
    doGetState(filterNode);
    eventQueueMutex.unlock();
}


bool BaseFilter::hasFrames() 
{
	if (!demandOriginFrames() || !demandDestinationFrames()) {
		return false;
	}
	return true;
}

OneToOneFilter::OneToOneFilter(bool force_) : 
BaseFilter(1, 1, force_)
{
}

bool OneToOneFilter::processFrame(bool removeFrame)
{
    bool newData = false;
	Frame* origin;

    processEvent();

	if (!demandOriginFrames() || !demandDestinationFrames()) {
        	return false;
	}

    if (doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        addFrames();
    }

    if (removeFrame) {
    	removeFrames();
	}

    return true;
}

OneToManyFilter::OneToManyFilter(int writersNum, bool force_) : 
BaseFilter(1, writersNum, force_)
{
}

bool OneToManyFilter::processFrame(bool removeFrame)
{
    bool newData;

    processEvent();

    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }
    if (doProcessFrame(oFrames.begin()->second, dFrames)) {
        addFrames();
    }
	
	if (removeFrame) {
    	removeFrames();
	}

    return true;
}

HeadFilter::HeadFilter(int writersNum) : 
BaseFilter(0, writersNum, false)
{
    
}

void HeadFilter::pushEvent(Event e)
{
    std::string action = e.getAction();
    Jzon::Node* params = e.getParams();
    Jzon::Object outputNode;

    if (action.empty()) {
        return;
    }

    if (eventMap.count(action) <= 0) {
        return;
    }
    
    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}



TailFilter::TailFilter(int readersNum) : 
BaseFilter(readersNum, 0, false)
{

}

void TailFilter::pushEvent(Event e)
{
    std::string action = e.getAction();
    Jzon::Node* params = e.getParams();
    Jzon::Object outputNode;

    if (action.empty()) {
        return;
    }

    if (eventMap.count(action) <= 0) {
        return;
    }
    
    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}


ManyToOneFilter::ManyToOneFilter(int readersNum, bool force_) : 
BaseFilter(readersNum, 1, force_)
{
}

bool ManyToOneFilter::processFrame(bool removeFrame)
{
    bool newData;

    processEvent();

    if (!demandOriginFrames() || !demandDestinationFrames()) {
        return false;
    }

    if (doProcessFrame(oFrames, dFrames.begin()->second)) {
        addFrames();
    }

	if (removeFrame) {
    	removeFrames();
	}
    
    return true;
}
