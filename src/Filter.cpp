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

#define WALL_CLOCK_THRESHOLD 100000 //us
#define SLOW_MODIFIER 1.10 
#define FAST_MODIFIER 0.90 

BaseFilter::BaseFilter(unsigned maxReaders_, unsigned maxWriters_, bool force_) : 
    maxReaders(maxReaders_), maxWriters(maxWriters_), force(force_), enabled(true)
{
    frameTime = std::chrono::microseconds(0);
    frameTimeMod = 1;
    bufferStateFrameTimeMod = 1;
}

BaseFilter::~BaseFilter()
{
    for (auto it : readers) {
        delete it.second;
    }

    for (auto it : writers) {
        delete it.second;
    }

    readers.clear();
    writers.clear();
    oFrames.clear();
    dFrames.clear();
    rUpdates.clear();
}

std::chrono::microseconds BaseFilter::getFrameTime()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(frameTime*frameTimeMod*bufferStateFrameTimeMod);
}


Reader* BaseFilter::getReader(int id) 
{
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}

Reader* BaseFilter::setReader(int readerID, FrameQueue* queue, bool sharedQueue)
{
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader(sharedQueue);
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
    bool newFrame;
    bool someFrame = false;
    QueueState qState;
    
    for (auto it : readers) {
        if (!it.second->isConnected()) {
            it.second->disconnect();
            //NOTE: think about readers as shared pointers
            delete it.second;
            readers.erase(it.first);
            continue;
        }

        oFrames[it.first] = it.second->getFrame(qState, newFrame, force);

        if (oFrames[it.first] != NULL) {
            if (newFrame) {
                rUpdates[it.first] = true;
            } else {
                rUpdates[it.first] = false;
            }

            someFrame = true;
        
        } else {
            rUpdates[it.first] = false;
        }

    }

    if (qState == SLOW) {
        bufferStateFrameTimeMod = SLOW_MODIFIER;
    } else {
        bufferStateFrameTimeMod = FAST_MODIFIER;
    }

    return someFrame;
}

bool BaseFilter::demandDestinationFrames()
{
    bool newFrame = false;
    for (auto it : writers){
        if (!it.second->isConnected()){
            it.second->disconnect();
            delete it.second;
            writers.erase(it.first);
            continue;
        }

        dFrames[it.first] = it.second->getFrame(true);
        newFrame = true;
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
    
    if (!(r = R->setReader(readerID, queue, slaveQueue))) {
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
    int writerID = generateWriterID();
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
    int writerID = generateWriterID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::disconnectWriter(int writerId)
{
    if (writers.count(writerId) <= 0) {
        return false;
    }

    writers[writerId]->disconnect();
    return true;
}

bool BaseFilter::disconnectReader(int readerId)
{
    if (readers.count(readerId) <= 0) {
        return false;
    }

    readers[readerId]->disconnect();
    return true;
}

void BaseFilter::disconnectAll()
{
    for (auto it : writers) {
        it.second->disconnect();
    }

    for (auto it : readers) {
        it.second->disconnect();
    }
}

bool BaseFilter::disconnect(BaseFilter *R, int writerId, int readerId)
{
    if (writers.count(writerId) <= 0) {
        return false;
    }

    Reader *r = R->getReader(readerId);

    if (!r) {
        return false;
    }

    writers[writerId]->disconnect(r);
    dFrames.erase(writerId);
    R->oFrames.erase(readerId);

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

        if (action.empty() || eventMap.count(action) <= 0) {
            outputNode.Add("error", "Error while processing event. Wrong action...");
            e.sendAndClose(outputNode);
            eventQueue.pop();
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
    filterNode.Add("workerId", workerId);
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

bool BaseFilter::deleteReader(int id)
{
    if (readers.count(id) <= 0) {
        return false;
    }

    if (readers[id]->isConnected()) {
        return false;
    }

    delete readers[id];
    readers.erase(id);

    return true;
}

void BaseFilter::updateTimestamp()
{
    if (frameTime.count() == 0) {
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        return;
    }

    timestamp += frameTime;

    lastDiffTime = diffTime;
    diffTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch() - timestamp);

    if (diffTime.count() > WALL_CLOCK_THRESHOLD || diffTime.count() < (-WALL_CLOCK_THRESHOLD) ) {
        // reset timestamp value in order to realign with the wall clock
        utils::warningMsg("Wall clock deviations exceeded! Reseting values!");
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        diffTime = std::chrono::microseconds(0);
        frameTimeMod = 1;
    }

    if (diffTime.count() > 0 && lastDiffTime < diffTime) {
        // delayed and incrementing delay. Need to speed up
        frameTimeMod -= 0.01;
    }

    if (diffTime.count() < 0 && lastDiffTime > diffTime) {
        // advanced and incremeting advance. Need to slow down
        frameTimeMod += 0.01;
    }

    if (frameTimeMod < 0) {
        frameTimeMod = 0;
    }
}

OneToOneFilter::OneToOneFilter(bool force_) : 
BaseFilter(1, 1, force_)
{
}

bool OneToOneFilter::processFrame(bool removeFrame)
{
    if (!demandOriginFrames() || !demandDestinationFrames()) {
            return false;
    }

    if (doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        updateTimestamp();
        dFrames.begin()->second->setPresentationTime(timestamp);
        addFrames();
    }
    
    if (removeFrame) {
        removeFrames();
    }

    return true;
}


OneToManyFilter::OneToManyFilter(unsigned writersNum, bool force_) : 
BaseFilter(1, writersNum, force_)
{
}

bool OneToManyFilter::processFrame(bool removeFrame)
{
    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }

    if (doProcessFrame(oFrames.begin()->second, dFrames)) {
        updateTimestamp();

        for (auto it : dFrames) {
            it.second->setPresentationTime(timestamp);
        }
 
        addFrames();
    }

    if (removeFrame) {
    	removeFrames();
    }

    return true;
}

HeadFilter::HeadFilter(unsigned writersNum) : 
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



TailFilter::TailFilter(unsigned readersNum) : 
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


ManyToOneFilter::ManyToOneFilter(unsigned readersNum, bool force_) : 
BaseFilter(readersNum, 1, force_)
{
}

bool ManyToOneFilter::processFrame(bool removeFrame)
{
    if (!demandOriginFrames() || !demandDestinationFrames()) {
        return false;
    }

    if (doProcessFrame(oFrames, dFrames.begin()->second)) {
        updateTimestamp();
        dFrames.begin()->second->setPresentationTime(timestamp);
        addFrames();
    }

    if (removeFrame) {
    	removeFrames();
    }
    
    return true;
}
