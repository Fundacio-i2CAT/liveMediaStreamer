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

#define WAIT 1000 //usec


BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, FilterRole fRole_, bool periodic): Runnable(periodic), 
process(false), maxReaders(readersNum), maxWriters(writersNum),  frameTime(std::chrono::microseconds(0)), 
fRole(fRole_), syncTs(std::chrono::microseconds(0))
{

}

BaseFilter::~BaseFilter()
{
    std::lock_guard<std::mutex> guard(mtx);
    for (auto it : readers) {
        it.second->removeReader(getId());
    }

    readers.clear();
    writers.clear();
}

bool BaseFilter::isWConnected (int wId) 
{
    std::lock_guard<std::mutex> guard(mtx);

    if (writers.count(wId) > 0 && writers[wId]->isConnected()){
        return true;
    }
    
    return false;
}

ConnectionData BaseFilter::getWConnectionData (int wId) 
{   
    std::lock_guard<std::mutex> guard(mtx);

    if (writers.count(wId) > 0 && writers[wId]->isConnected()){
        return writers[wId]->getCData();
    }
    
    ConnectionData cData;
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
    
    if (!specificReaderConfig(readerID, queue)){
        r.reset();
    }
    
    readers[readerID] = r;

    return r;
}

unsigned BaseFilter::generateReaderID()
{
    std::lock_guard<std::mutex> guard(mtx);
    if (maxReaders == 1) {
        return DEFAULT_ID;
    }

    unsigned id = rand();

    while (readers.count(id) > 0) {
        id = rand();
    }

    return id;
}

unsigned BaseFilter::generateWriterID()
{
    std::lock_guard<std::mutex> guard(mtx);
    if (maxWriters == 1) {
        return DEFAULT_ID;
    }

    unsigned id = rand();

    while (writers.count(id) > 0) {
        id = rand();
    }

    return id;
}

bool BaseFilter::demandDestinationFrames(std::map<int, Frame*> &dFrames)
{
    std::lock_guard<std::mutex> guard(mtx);
    
    if (maxWriters == 0) {
        return true;
    }

    bool newFrame = false;
    for (auto it : writers){
        if (!it.second->isConnected()){
            it.second->disconnect();
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

std::vector<int> BaseFilter::addFrames(std::map<int, Frame*> &dFrames)
{
    std::lock_guard<std::mutex> guard(mtx);    
    std::vector<int> enabledJobs;
    
    for (auto it : dFrames){
        if (it.second->getConsumed()) {
            int wId = it.first;
            if (writers.count(wId) > 0 && writers[wId]->isConnected()){
                enabledJobs.push_back(writers[wId]->addFrame());
            }
        }
    }

    return enabledJobs;
}

bool BaseFilter::removeFrames(std::vector<int> framesToRemove)
{
    bool executeAgain = false;

    if (maxReaders == 0) {
        return executeAgain;
    }
    
    std::lock_guard<std::mutex> guard(mtx);
    
    for (auto id : framesToRemove){
        if (readers.count(id) > 0){
            readers[id]->removeFrame(getId());
            executeAgain |= (readers[id]->getQueueElements() > 0);
        }
    }

    return executeAgain;
}

bool BaseFilter::shareReader(BaseFilter *shared, int sharedRId, int orgRId)
{
    std::lock_guard<std::mutex> guard(mtx);  
    
    if (getId() == shared->getId()){
        utils::errorMsg("Shared filter and base filter are the same!!");
        return false;
    }
    
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
    
    if (!shared->specificReaderConfig(sharedRId, readers[orgRId]->getQueue())){
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
    ConnectionData cData;
    
    std::lock_guard<std::mutex> guard(mtx);
      
    if (writers.size() >= maxWriters) {
        utils::errorMsg("Too many writers!");
        return false;
    }
    
    if (writers.count(writerID) > 0){
        utils::warningMsg("Writer id must be unique");
        return false;
    }
    
    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        utils::errorMsg("Reader " + std::to_string(readerID) + " null or already connected");
        return false;
    }

    writers[writerID] = std::shared_ptr<Writer>(new Writer());
    seqNums[writerID] = 0;

    cData.wFilterId = getId();
    cData.writerId = writerID;
    cData.rFilterId = R->getId();
    cData.readerId = readerID;
    
    queue = allocQueue(cData);
    if (!queue){
        writers.erase(writerID);
        return false;
    }

    if (!(r = R->setReader(readerID, queue))) {
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
    std::lock_guard<std::mutex> guard(mtx);
    
    if (writers.count(writerId) <= 0) {
        utils::warningMsg("Required writer does not exist");
        return true;
    }

    if (writers[writerId]->disconnect()){
        writers.erase(writerId);
        return true;
    }
    
    return false;
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
        case REGULAR:
            enabledJobs = regularProcessFrame(ret);
            break;
        case SERVER:
            enabledJobs = serverProcessFrame(ret);
            break;
        default:
            ret = WAIT;
            break;
    }

    return enabledJobs;
}


std::vector<int> BaseFilter::regularProcessFrame(int& ret)
{
    std::vector<int> enabledJobs;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    std::vector<int> newFrames;
    
    processEvent();
    
    newFrames = demandOriginFrames(oFrames);
    
    if (newFrames.empty()) {
        ret = WAIT;
        return enabledJobs;
    }
    
    if (!demandDestinationFrames(dFrames)) {
        ret = WAIT;
        return enabledJobs;
    }

    runDoProcessFrame(oFrames, dFrames, newFrames);

    //TODO: manage ret value
    enabledJobs = addFrames(dFrames);
    
    if (removeFrames(newFrames)) {
        enabledJobs.push_back(getId());
    }

    return enabledJobs;
}

std::vector<int> BaseFilter::serverProcessFrame(int& ret)
{
    std::vector<int> enabledJobs;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    std::vector<int> newFrames;
    
    processEvent();
    
    newFrames = demandOriginFrames(oFrames);
    demandDestinationFrames(dFrames);

    runDoProcessFrame(oFrames, dFrames, newFrames);

    enabledJobs = addFrames(dFrames);
    if (removeFrames(newFrames)){
        enabledJobs.push_back(getId());
    }
    
    ret = 0;
    
    return enabledJobs;
}

std::vector<int> BaseFilter::demandOriginFrames(std::map<int, Frame*> &oFrames)
{
    if (maxReaders == 0) {
        return std::vector<int>({-1});
    }

    if (readers.empty()) {
        return std::vector<int>({});
    }

    if (frameTime.count() <= 0) {
        return demandOriginFramesBestEffort(oFrames);
    } else {
        return demandOriginFramesFrameTime(oFrames);
    }
}

bool BaseFilter::deleteReader(int readerId)
{
    if (readers.count(readerId) > 0){
        readers[readerId]->removeReader(getId());
        readers.erase(readerId);
        return specificReaderDelete(readerId);
    }
    
    return false;
}

std::vector<int> BaseFilter::demandOriginFramesBestEffort(std::map<int, Frame*> &oFrames) 
{
    bool newFrame;
    std::vector<int> newFrames;
    Frame* frame;

    for (auto r : readers) {
        if (!r.second->isConnected()) {
            deleteReader(r.first);
            continue;
        }
        
        frame = r.second->getFrame(getId(), newFrame);

        if (!frame) {
            utils::errorMsg("[BaseFilter::demandOriginFramesBestEffort] Reader->getFrame() returned NULL. It cannot happen...");
            continue;
        }

        frame->setConsumed(newFrame);
        oFrames[r.first] = frame;
        if (newFrame){
            newFrames.push_back(r.first);
        }
    }

    return newFrames;
}

std::vector<int> BaseFilter::demandOriginFramesFrameTime(std::map<int, Frame*> &oFrames) 
{
    Frame* frame;
    std::chrono::microseconds outOfScopeTs = std::chrono::microseconds(-1);
    bool newFrame;
    bool validFrame = false;
    bool outDated = false;
    std::vector<int> newFrames;

    for (auto r : readers) {
        if (!r.second->isConnected()) {
            deleteReader(r.first);
            continue;
        }

        frame = r.second->getFrame(getId(), newFrame);

        if (!frame) {
            utils::errorMsg("[BaseFilter::demandOriginFramesFrameTime] Reader->getFrame() returned NULL. It cannot happen...");
            continue;
        }

        frame->setConsumed(newFrame);
        oFrames[r.first] = frame;
        if (newFrame){
            newFrames.push_back(r.first);
        }

        if (!newFrame) {
            continue;
        }

        // If the current frame is out of our mixing scope, 
        // we do not consider it as a new mixing frame (keep noFrame value)
        if (frame->getPresentationTime() > syncTs + frameTime) {
            if (outOfScopeTs.count() < 0) {
                outOfScopeTs = frame->getPresentationTime();
            } else {
                outOfScopeTs = std::min(frame->getPresentationTime(), outOfScopeTs);
            }
            continue;
        }
        
        if (frame->getPresentationTime() < syncTs){
            outDated = true;
            continue;
        }

        // Normal case, which means that the frame is in our mixing scope (syncTime -> syncTime+frameTime)
        validFrame = true;
    }

    // There is no new frame
    if (newFrames.empty()) {
        return newFrames;
    }
    
    if (!validFrame) {
        if (outOfScopeTs > syncTs && !outDated) {
            syncTs = outOfScopeTs;
        }
        //We return true if there are no outDated frames
        return newFrames;
    } 

    // Finally set syncTs
    syncTs += frameTime;
    return newFrames;
}

OneToOneFilter::OneToOneFilter(FilterRole fRole_, bool periodic) :
    BaseFilter(1, 1, fRole_, periodic)
{
}

bool OneToOneFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/)
{
    std::chrono::microseconds outTimestamp;

    if (!doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        return false;
    }

    dFrames.begin()->second->setOriginTime(oFrames.begin()->second->getOriginTime());
    dFrames.begin()->second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    return true;
}


OneToManyFilter::OneToManyFilter(unsigned writersNum, FilterRole fRole_, bool periodic) :
    BaseFilter(1, writersNum, fRole_, periodic)
{
}

bool OneToManyFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/)
{
    if (!doProcessFrame(oFrames.begin()->second, dFrames)) {
        return false;
    }

    for (auto it : dFrames) {
        it.second->setOriginTime(oFrames.begin()->second->getOriginTime());
        it.second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
    }

    return true;
}


HeadFilter::HeadFilter(unsigned writersNum, FilterRole fRole_, bool periodic) :
    BaseFilter(0, writersNum, fRole_, periodic)
{
}

bool HeadFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/)
{
    if (!doProcessFrame(dFrames)) {
        return false;
    }

    for (auto it : dFrames) {
        if (it.second->getConsumed()){
            it.second->setOriginTime(std::chrono::high_resolution_clock::now());
            it.second->setSequenceNumber(seqNums[it.first]++);
        }
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

TailFilter::TailFilter(unsigned readersNum, FilterRole fRole_, bool periodic) :
    BaseFilter(readersNum, 0, fRole_, periodic)
{
}

bool TailFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames)
{
    return doProcessFrame(oFrames, newFrames);
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


ManyToOneFilter::ManyToOneFilter(unsigned readersNum, FilterRole fRole_, bool periodic) :
    BaseFilter(readersNum, 1, fRole_, periodic)
{
}

bool ManyToOneFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames)
{
    if (!doProcessFrame(oFrames, dFrames.begin()->second, newFrames)) {
        return false;
    }

    dFrames.begin()->second->setOriginTime(std::chrono::high_resolution_clock::now());
    dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]++);
    return true;
}

