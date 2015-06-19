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
 *            Gerard Castillo <gerard.castillo@i2cat.net>
 */

#include "Filter.hh"
#include "Utils.hh"

#include <thread>

BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, size_t fTime, FilterRole fRole_, bool sharedFrames_):
process(false), maxReaders(readersNum), maxWriters(writersNum), frameTime(fTime), fRole(fRole_), sharedFrames(sharedFrames_)
{

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

void BaseFilter::setFrameTime(std::chrono::microseconds fTime)
{
    frameTime = fTime;
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

bool BaseFilter::demandDestinationFrames()
{
    if (maxWriters == 0) {
        return true;
    }

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

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID)
{
    Reader* r;
    FrameQueue *queue;

    if (writers.size() < getMaxWriters() && writers.count(writerID) <= 0) {
        writers[writerID] = new Writer();
        seqNums[writerID] = 0;
        utils::debugMsg("New writer created " + std::to_string(writerID));
    }

    if (writers.count(writerID) > 0 && writers[writerID]->isConnected()) {
        utils::errorMsg("Writer " + std::to_string(writerID) + " null or already connected");
        return false;
    }

    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        utils::errorMsg("Reader " + std::to_string(readerID) + " null or already connected");
        return false;
    }

    queue = allocQueue(writerID);

    if (!(r = R->setReader(readerID, queue))) {
        utils::errorMsg("Could not set the queue to the reader");
        return false;
    }

    writers[writerID]->setQueue(queue);

    return writers[writerID]->connect(r);
}

bool BaseFilter::connectOneToOne(BaseFilter *R)
{
    int writerID = generateWriterID();
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID);
}

bool BaseFilter::connectManyToOne(BaseFilter *R, int writerID)
{
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID);
}

bool BaseFilter::connectManyToMany(BaseFilter *R, int readerID, int writerID)
{
    return connect(R, writerID, readerID);
}

bool BaseFilter::connectOneToMany(BaseFilter *R, int readerID)
{
    int writerID = generateWriterID();
    return connect(R, writerID, readerID);
}

bool BaseFilter::disconnectWriter(int writerId)
{
    bool ret;
    if (writers.count(writerId) <= 0) {
        return false;
    }

    ret = writers[writerId]->disconnect();
    if (ret){
        writers.erase(writerId);
    }
    return ret;
}

bool BaseFilter::disconnectReader(int readerId)
{
    bool ret;
    if (readers.count(readerId) <= 0) {
        return false;
    }

    ret = readers[readerId]->disconnect();
    if (ret){
        readers.erase(readerId);
    }
    return ret;
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

void BaseFilter::processEvent()
{
    std::string action;
    Jzon::Node* params;

    std::lock_guard<std::mutex> guard(eventQueueMutex);

    while(newEvent()) {

        Event e = eventQueue.top();
        action = e.getAction();
        params = e.getParams();

        if (action.empty() || eventMap.count(action) <= 0) {
            utils::errorMsg("Wrong action name while processing event in filter");
            eventQueue.pop();
            continue;
        }

        if (!eventMap[action](params)) {
            utils::errorMsg("Error executing filter event");
        }
        
        eventQueue.pop();
    }
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
    std::lock_guard<std::mutex> guard(eventQueueMutex);
    eventQueue.push(e);
}

void BaseFilter::getState(Jzon::Object &filterNode)
{
    std::lock_guard<std::mutex> guard(eventQueueMutex);
    filterNode.Add("type", utils::getFilterTypeAsString(fType));
    filterNode.Add("role", utils::getRoleAsString(fRole));
    filterNode.Add("workerId", workerId);
    doGetState(filterNode);
}

std::chrono::nanoseconds BaseFilter::processFrame()
{
    std::chrono::nanoseconds retValue;
    std::chrono::microseconds outTimestamp;

    switch(fRole) {
        case MASTER:
            retValue = masterProcessFrame();
            break;
        case SLAVE:
            retValue = slaveProcessFrame();
            break;
        case NETWORK:
            runDoProcessFrame(outTimestamp);
            retValue = std::chrono::nanoseconds(0);
            break;
        default:
            retValue = std::chrono::nanoseconds(RETRY);
            break;
    }

    return retValue;
}

void BaseFilter::processAll()
{
    for (auto it : slaves) {
        if (sharedFrames){
            it.second->updateFrames(oFrames);
        }
        it.second->execute();
    }
}

bool BaseFilter::runningSlaves()
{
    bool running = false;
    for (auto it : slaves){
        running |= it.second->isProcessing();
    }
    return running;
}

void BaseFilter::setSharedFrames(bool sharedFrames_)
{
    if(fRole == MASTER){
        sharedFrames = sharedFrames_;
    }
}

bool BaseFilter::addSlave(int id, BaseFilter *slave)
{
    if (fRole != MASTER || slave->fRole != SLAVE){
        return false;
    }

    if (slaves.count(id) > 0){
        return false;
    }

    slave->sharedFrames = sharedFrames;

    slaves[id] = slave;

    return true;
}

std::chrono::nanoseconds BaseFilter::masterProcessFrame()
{
    std::chrono::nanoseconds enlapsedTime;
    std::chrono::nanoseconds frameTime_;
    std::chrono::microseconds outTimestamp;
    
    processEvent();
      
    if (!demandOriginFrames(outTimestamp) || !demandDestinationFrames()) {
        return std::chrono::nanoseconds(RETRY);
    }

    processAll();

    runDoProcessFrame(outTimestamp);

    while (runningSlaves()){
        std::this_thread::sleep_for(std::chrono::nanoseconds(RETRY));
    }

    removeFrames();

    return std::chrono::nanoseconds(0);
}

std::chrono::nanoseconds BaseFilter::slaveProcessFrame()
{
    std::chrono::microseconds outTimestamp;

    if (!process) {
        return std::chrono::nanoseconds(RETRY);
    }

    processEvent();

    //TODO: decide policy to set run to true/false if retry
    if (!demandDestinationFrames()){
        return std::chrono::nanoseconds(RETRY);
    }

    if (!sharedFrames && !demandOriginFrames(outTimestamp)) {
        return std::chrono::nanoseconds(RETRY);
    }

    runDoProcessFrame(outTimestamp);

    if (!sharedFrames){
        removeFrames();
    }

    process = false;
    return std::chrono::nanoseconds(RETRY);
}

void BaseFilter::updateFrames(std::map<int, Frame*> oFrames_)
{
    oFrames = oFrames_;
}

bool BaseFilter::demandOriginFrames(std::chrono::microseconds &outTimestamp)
{
    bool success;

    if (readers.empty()) {
        return false;
    }

    if (frameTime.count() <= 0) {
        success = demandOriginFramesBestEffort(outTimestamp);
    } else {
        success = demandOriginFramesFrameTime(outTimestamp);
    }

    return success;
}

bool BaseFilter::demandOriginFramesBestEffort(std::chrono::microseconds &outTimestamp) 
{
    std::chrono::microseconds outTs = std::chrono::microseconds(0);
    bool noFrame = true;
    Frame* frame;

    for (auto r : readers) {
        frame = r.second->getFrame();

        if (!frame) {
            frame = r.second->getFrame(true);
            oFrames[r.first] = frame;
            rUpdates[r.first] = false;
            continue;
        }

        outTs = std::max(frame->getPresentationTime(), outTs);
        oFrames[r.first] = frame;
        rUpdates[r.first] = true;
        noFrame = false;
    }

    if (noFrame) {
        return false;
    }

    outTimestamp = outTs;
    return false;
}

bool BaseFilter::demandOriginFramesFrameTime(std::chrono::microseconds &outTimestamp) 
{
    std::chrono::microseconds outOfScopeTs = std::chrono::microseconds(-1);
    std::chrono::microseconds outTs = std::chrono::microseconds(-1);
    Frame* frame;

    for (auto r : readers) {
        frame = r.second->getFrame();

        // In case a frame is earliear than syncTs, we will discard frames
        // until we found a valid one or until we found a NULL (which will treat later) 
        while(frame && frame->getPresentationTime() < syncTs) {
            r.second->removeFrame();
            frame = r.second->getFrame();
        }

        // If there is no frame or the current one is out of our mixing scope, 
        // we get the previous one from the queue
        if (!frame || frame->getPresentationTime() >= syncTs + frameTime) {
            frame = r.second->getFrame(true);
            oFrames[r.first] = frame;
            rUpdates[r.first] = false;

            if (outOfScopeTs.count() < 0) {
                outOfScopeTs = frame->getPresentationTime();
            } else {
                outOfScopeTs = std::min(frame->getPresentationTime(), outOfScopeTs);
            }

            continue;
        }

        // Normal case, which means that the frame is in our mixing scope (syncTime -> syncTime+frameTime)
        // We use its timestamp to calculate the output frame timestamp 
        if (outTs.count() < 0) {
            outTs = frame->getPresentationTime();
        } else {
            outTs = std::min(frame->getPresentationTime(), outTs);
        }

        oFrames[r.first] = frame;
        rUpdates[r.first] = true;
    }

    // After all, there was no valid frame, so we return false
    // This case is nearly impossible
    if (outTs.count() < 0) {

        if (outOfScopeTs.count() > 0) {
            syncTs = outOfScopeTs;
        }

        return false;
    } 

    // Finally set timestamp and set syncTs
    outTimestamp = syncTs;
    syncTs += frameTime;
    return true;
}


OneToOneFilter::OneToOneFilter(FilterRole fRole_, bool sharedFrames_, size_t fTime) :
BaseFilter(1, 1, fTime, fRole_, sharedFrames_)
{
    
}

bool OneToOneFilter::runDoProcessFrame(std::chrono::microseconds outTimestamp)
{
    if (!doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        return false;
    }
    
    dFrames.begin()->second->setPresentationTime(outTimestamp);
    dFrames.begin()->second->setDuration(oFrames.begin()->second->getDuration());
    dFrames.begin()->second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    addFrames();
    return true;
}

OneToManyFilter::OneToManyFilter(FilterRole fRole_, bool sharedFrames_, unsigned writersNum, size_t fTime) :
BaseFilter(1, writersNum, fTime, fRole_, sharedFrames_)
{

}

bool OneToManyFilter::runDoProcessFrame(std::chrono::microseconds outTimestamp)
{
    if (!doProcessFrame(oFrames.begin()->second, dFrames)) {
        return false;
    }

    for (auto it : dFrames) {
        it.second->setPresentationTime(outTimestamp);
        it.second->setDuration(oFrames.begin()->second->getDuration());
        it.second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    }

    addFrames();
    return true;
}

HeadFilter::HeadFilter(FilterRole fRole_, size_t fTime) :
BaseFilter(0, 1, fTime, fRole_, false)
{
    
}

bool HeadFilter::runDoProcessFrame(std::chrono::microseconds outTimestamp)
{
    if (doProcessFrame(dFrames.begin()->second)) {
        // dFrames.begin()->second->setPresentationTime(timestamp);
        // dFrames.begin()->second->setDuration(duration);
        seqNums[dFrames.begin()->first]++;
        dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]);
        addFrames();
        return true;
    }

    return false;
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

    if (!eventMap[action](params)) {
        utils::errorMsg("Error executing filter event");
    }
}

TailFilter::TailFilter(FilterRole fRole_, bool sharedFrames_, unsigned readersNum, size_t fTime) :
BaseFilter(readersNum, 0, fTime, fRole_, sharedFrames_)
{

}

bool TailFilter::runDoProcessFrame(std::chrono::microseconds outTimestamp)
{
    return doProcessFrame(oFrames);
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

    if (!eventMap[action](params)) {
        utils::errorMsg("Error executing filter event");
    }
}

ManyToOneFilter::ManyToOneFilter(FilterRole fRole_, bool sharedFrames_, unsigned readersNum, size_t fTime) :
BaseFilter(readersNum, 1, fTime, fRole_, sharedFrames_)
{

}

bool ManyToOneFilter::runDoProcessFrame(std::chrono::microseconds outTimestamp)
{
    if (doProcessFrame(oFrames, dFrames.begin()->second)) {
        //NOTE: this assignment is only done in order to advance in the timestamp refactor. Must be implmented correctly
        dFrames.begin()->second->setPresentationTime(oFrames.begin()->second->getPresentationTime());
        dFrames.begin()->second->setDuration(oFrames.begin()->second->getDuration());
        seqNums[dFrames.begin()->first]++;
        dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]);
        addFrames();
        return true;
    }

    return false;
}

LiveMediaFilter::LiveMediaFilter(unsigned readersNum, unsigned writersNum) :
BaseFilter(readersNum, writersNum, 0, NETWORK, false)
{

}

void LiveMediaFilter::pushEvent(Event e)
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

    if (!eventMap[action](params)) {
        utils::errorMsg("Error executing filter event");
    }
}

