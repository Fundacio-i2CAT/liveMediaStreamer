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


BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, FilterRole fRole_, bool periodic): Runnable(periodic), 
process(false), maxReaders(readersNum), maxWriters(writersNum),  frameTime(std::chrono::microseconds(0)), 
fRole(fRole_), syncTs(std::chrono::microseconds(0))
{
}

BaseFilter::~BaseFilter()
{
    std::lock_guard<std::mutex> guard(mtx);
    for (auto it : readers) {
        it.second->removeReader();
    }

    for (auto it : writers) {
        delete it.second;
    }

    readers.clear();
    writers.clear();
    oFrames.clear();
    dFrames.clear();
}

bool BaseFilter::isWConnected (int wId) 
{
    std::lock_guard<std::mutex> guard(mtx);

    if (writers.count(wId) > 0 && writers[wId]->isConnected()){
        return true;
    }
    
    return false;
}

struct ConnectionData BaseFilter::getWConnectionData (int wId) 
{   
    std::lock_guard<std::mutex> guard(mtx);

    if (writers.count(wId) > 0 && writers[wId]->isConnected()){
        return writers[wId]->getCData();
    }
    
    struct ConnectionData cData;
    return cData;
}

bool BaseFilter::isRConnected (int rId) 
{
    std::lock_guard<std::mutex> guard(mtx);

    if (readers.count(rId) > 0 && readers[rId]->isConnected()){
        return true;
    }
    
    return false;
}

void BaseFilter::setFrameTime(std::chrono::microseconds fTime)
{
    frameTime = fTime;
}

std::shared_ptr<Reader> BaseFilter::getReader(int id)
{
    std::lock_guard<std::mutex> guard(mtx);
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}

std::shared_ptr<Reader> BaseFilter::setReader(int readerID, FrameQueue* queue)
{
    std::lock_guard<std::mutex> guard(mtx);
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    std::shared_ptr<Reader> r (new Reader());
    readers[readerID] = r;

    return r;
}

int BaseFilter::generateReaderID()
{
    std::lock_guard<std::mutex> guard(mtx);
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
    std::lock_guard<std::mutex> guard(mtx);
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
    std::lock_guard<std::mutex> guard(mtx);
    
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

        Frame *f = it.second->getFrame(true);
        f->setConsumed(false);
        dFrames[it.first] = f;
        newFrame = true;
    }

    return newFrame;
}

std::vector<int> BaseFilter::addFrames()
{
    std::lock_guard<std::mutex> guard(mtx);
    
    std::vector<int> enabledJobs;
    
    for (auto it : dFrames){
        if (it.second->getConsumed()) {
            int wId = it.first;
            if (writers[wId]->isConnected()){
                enabledJobs.push_back(writers[wId]->addFrame());
            }
        }
    }

    return enabledJobs;
}

std::vector<int> BaseFilter::removeFrames()
{
    std::vector<int> enabledJobs;
    
    if (maxReaders == 0){
        return enabledJobs;
    }
    
    std::lock_guard<std::mutex> guard(mtx);
    
    for (auto it : readers){
        if (oFrames[it.first] && oFrames[it.first]->getConsumed()){
            enabledJobs.push_back(it.second->removeFrame());
        }
    }

    return enabledJobs;
}

bool BaseFilter::shareReader(BaseFilter *shared, int sharedRId, int orgRId)
{
    std::lock_guard<std::mutex> guard(mtx);
    
    if (!readers.count(orgRId) > 0) {
        utils::errorMsg("The reader to share is not there");
        return false;
    }
    if (!readers[orgRId]->isConnected()){
        utils::errorMsg("The reader to share is not connected!");
        return false;
    }
    if (shared->isRConnected(sharedRId)){
        utils::errorMsg("The shared filter has the reader already connected!");
        return false;
    }
    
    shared->readers[sharedRId] = readers[orgRId];
    readers[orgRId]->addReader();
    
    return true;
}

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID)
{
    std::shared_ptr<Reader> r;
    FrameQueue *queue = NULL;
    struct ConnectionData cData;
    
    std::lock_guard<std::mutex> guard(mtx);
      
    if (writers.size() >= maxWriters) {
        utils::errorMsg("Too many writers!");
        return false;
    }
    
    if (writers.count(writerID) > 0){
        utils::errorMsg("Writer id must be unique");
        return false;
    }
    
    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        utils::errorMsg("Reader " + std::to_string(readerID) + " null or already connected");
        return false;
    }

    writers[writerID] = new Writer();
    seqNums[writerID] = 0;

    cData.wFilterId = getId();
    cData.writerId = writerID;
    cData.rFilterId = R->getId();
    cData.readerId = readerID;
    
    queue = allocQueue(cData);
    if (!queue){
        delete writers[writerID];
        writers.erase(writerID);
        return false;
    }

    if (!(r = R->setReader(readerID, queue))) {
        delete writers[writerID];
        writers.erase(writerID);
        utils::errorMsg("Could not create the reader or set the queue");
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
    
    std::lock_guard<std::mutex> guard(mtx);
    
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
    
    std::lock_guard<std::mutex> guard(mtx);
    
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
    std::lock_guard<std::mutex> guard(mtx);
    
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

    std::lock_guard<std::mutex> guard(mtx);

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
    std::lock_guard<std::mutex> guard(mtx);
    eventQueue.push(e);
}

void BaseFilter::getState(Jzon::Object &filterNode)
{
    std::lock_guard<std::mutex> guard(mtx);
    filterNode.Add("type", utils::getFilterTypeAsString(fType));
    filterNode.Add("role", utils::getRoleAsString(fRole));
    doGetState(filterNode);
}

std::vector<int> BaseFilter::processFrame(int& ret)
{
    std::vector<int> enabledJobs;

    switch(fRole) {
        case MASTER:
            enabledJobs = masterProcessFrame(ret);
            break;
        case SLAVE:
            break;
        case SERVER:
            enabledJobs = serverProcessFrame(ret);
            break;
        default:
            ret = 0;
            break;
    }

    return enabledJobs;
}


std::vector<int> BaseFilter::masterProcessFrame(int& ret)
{
    std::chrono::microseconds enlapsedTime;
    std::chrono::microseconds frameTime_;
    std::vector<int> enabledJobs;
    
    processEvent();
     
    if (!demandOriginFrames()) {
        ret = 0;
        return enabledJobs;
    }
    
    if (!demandDestinationFrames()) {
        ret = 0;
        return enabledJobs;
    }

    runDoProcessFrame();


    enabledJobs = addFrames();
    removeFrames();

    return enabledJobs;
}

std::vector<int> BaseFilter::serverProcessFrame(int& ret)
{
    std::vector<int> enabledJobs;

    processEvent();
      
    demandOriginFrames();
    demandDestinationFrames();

    runDoProcessFrame();

    enabledJobs = addFrames();
    removeFrames();
    
    ret = 0;
    
    return enabledJobs;
}

void BaseFilter::updateFrames(std::map<int, Frame*> oFrames_)
{
    oFrames = oFrames_;
}

bool BaseFilter::demandOriginFrames()
{
    bool success;

    if (maxReaders == 0) {
        return true;
    }

    if (readers.empty()) {
        return false;
    }

    if (frameTime.count() <= 0) {
        success = demandOriginFramesBestEffort();
    } else {
        success = demandOriginFramesFrameTime();
    }

    return success;
}

bool BaseFilter::deleteReader(int readerId)
{
    if (readers.count(readerId) > 0){
        readers[readerId]->removeReader();
        readers.erase(readerId);
        return true;
    }
    
    return false;
}

bool BaseFilter::demandOriginFramesBestEffort() 
{
    bool someFrame = false;
    Frame* frame;

    for (auto r : readers) {
        if (!r.second->isConnected()) {
            deleteReader(r.first);
            continue;
        }
        
        frame = r.second->getFrame();

        while(frame && frame->getPresentationTime() < syncTs) {
            r.second->removeFrame();
            frame = r.second->getFrame();
        }

        if (!frame) {
            frame = r.second->getFrame(true);
            frame->setConsumed(false);
            oFrames[r.first] = frame;
            continue;
        }

        frame->setConsumed(true);
        oFrames[r.first] = frame;
        someFrame = true;
    }

    return someFrame;
}

bool BaseFilter::demandOriginFramesFrameTime() 
{
    Frame* frame;
    std::chrono::microseconds outOfScopeTs = std::chrono::microseconds(-1);
    bool noFrame = true;

    for (auto r : readers) {
        if (!r.second->isConnected()) {
            deleteReader(r.first);
            continue;
        }

        frame = r.second->getFrame();

        // In case a frame is earliear than syncTs, we will discard frames
        // until we found a valid one or until we found a NULL (which will treat later) 
        while(frame && frame->getPresentationTime() < syncTs) {
            r.second->removeFrame();
            frame = r.second->getFrame();
        }

        // If there is no frame get the previous one from the queue
        if (!frame) {
            frame = r.second->getFrame(true);
            frame->setConsumed(false);
            oFrames[r.first] = frame;
            continue;
        }

        // If the current frame is out of our mixing scope, 
        // we get the previous one from the queue
        if (frame->getPresentationTime() >= syncTs + frameTime) {
            frame->setConsumed(false);
            oFrames[r.first] = frame;

            if (outOfScopeTs.count() < 0) {
                outOfScopeTs = frame->getPresentationTime();
            } else {
                outOfScopeTs = std::min(frame->getPresentationTime(), outOfScopeTs);
            }

            continue;
        }

        // Normal case, which means that the frame is in our mixing scope (syncTime -> syncTime+frameTime)
        frame->setConsumed(true);
        oFrames[r.first] = frame;
        noFrame = false;
    }

    // After all, there was no valid frame, so we return false
    // This case is nearly impossible
    if (noFrame) {
        if (outOfScopeTs.count() > 0) {
            syncTs = outOfScopeTs;
        }
        return false;
    } 

    // Finally set syncTs
    syncTs += frameTime;
    return true;
}

OneToOneFilter::OneToOneFilter(FilterRole fRole_, bool periodic) :
    BaseFilter(1, 1, fRole_, periodic)
{
}

bool OneToOneFilter::runDoProcessFrame()
{
    std::chrono::microseconds outTimestamp;

    if (!doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        return false;
    }

    if (frameTime.count() <= 0) {
        outTimestamp = oFrames.begin()->second->getPresentationTime();
        setSyncTs(outTimestamp);
    } else {
        outTimestamp = getSyncTs();
    }
    
    dFrames.begin()->second->setPresentationTime(outTimestamp);
    dFrames.begin()->second->setDuration(oFrames.begin()->second->getDuration());
    dFrames.begin()->second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    return true;
}


OneToManyFilter::OneToManyFilter(FilterRole fRole_, unsigned writersNum, bool periodic) :
    BaseFilter(1, writersNum, fRole_, periodic)
{
}

bool OneToManyFilter::runDoProcessFrame()
{
    if (!doProcessFrame(oFrames.begin()->second, dFrames)) {
        return false;
    }

	//TODO: implement timestamp setting
    for (auto it : dFrames) {
        // it.second->setPresentationTime(outTimestamp);
        // it.second->setDuration(oFrames.begin()->second->getDuration());
        it.second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    }

    return true;
}


HeadFilter::HeadFilter(FilterRole fRole_, unsigned writersNum, bool periodic) :
    BaseFilter(0, writersNum, fRole_, periodic)
{
}

bool HeadFilter::runDoProcessFrame()
{
    if (!doProcessFrame(dFrames)) {
        return false;
    }

   for (auto it : dFrames) {
       it.second->setSequenceNumber(seqNums[it.first]++);
   }

   return true;
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

TailFilter::TailFilter(FilterRole fRole_, unsigned readersNum, bool periodic) :
    BaseFilter(readersNum, 0, fRole_, periodic)
{
}

bool TailFilter::runDoProcessFrame()
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


ManyToOneFilter::ManyToOneFilter(FilterRole fRole_, unsigned readersNum, bool periodic) :
    BaseFilter(readersNum, 1, fRole_, periodic)
{
}

bool ManyToOneFilter::runDoProcessFrame()
{
    if (!doProcessFrame(oFrames, dFrames.begin()->second)) {
        return false;
    }

    //TODO: duration??
    // dFrames.begin()->second->setDuration(oFrames.begin()->second->getDuration());
    seqNums[dFrames.begin()->first]++;
    dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]);
    return true;
}

