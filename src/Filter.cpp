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
#include <algorithm>

#define WAIT 1000 //usec


BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, FilterRole fRole_, bool periodic): Runnable(periodic), 
maxReaders(readersNum), maxWriters(writersNum),  frameTime(std::chrono::microseconds(0)), 
fRole(fRole_), syncTs(std::chrono::microseconds(0)), refReader(0),  syncMargin(std::chrono::microseconds(0))
{
}

BaseFilter::~BaseFilter()
{
    std::lock_guard<std::mutex> guard(mtx);
    for (auto it : readers) {
        if (it.second && it.first >= 0){
            it.second->removeReader(getId());
        }
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

std::chrono::microseconds BaseFilter::getAvgReaderDelay (int rId)
{
    std::shared_ptr<Reader> r = getReader(rId);

    if (!r) {
        return std::chrono::microseconds(-1);
    }

    return r->getAvgDelay();
}

size_t BaseFilter::getLostBlocs (int rId)
{
    std::shared_ptr<Reader> r = getReader(rId);

    if (!r) {
        return 0;
    }

    return r->getLostBlocs();
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
    for (std::map<int, std::shared_ptr<Writer>>::iterator it = writers.begin() ; it != writers.end(); ) {
        if (!it->second->isConnected()){
            it->second->disconnect();
            int wId = it->first;
            ++it;
            deleteWriter(wId);
            continue;
        }

        Frame *f = it->second->getFrame(true);
        f->setConsumed(false);
        dFrames[it->first] = f;
        newFrame = true;
        ++it;
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
                std::vector<int> addFrameReturn = writers[wId]->addFrame();
                enabledJobs.insert(enabledJobs.begin(), addFrameReturn.begin(), addFrameReturn.end());
            }
        }
    }

    return enabledJobs;
}

bool BaseFilter::removeFrames(std::vector<int> framesToRemove)
{
    bool removed = true;
    
    if (maxReaders == 0) {
        return removed;
    }
    
    std::lock_guard<std::mutex> guard(mtx);
    
    for (auto id : framesToRemove){
        if (readers.count(id) > 0){
            readers[id]->removeFrame(getId());
        } else {
            removed = false;
        }
    }

    return removed;
}

bool BaseFilter::pendingJobs()
{
    for (auto it : readers){
        if (it.second && it.second->getQueueElements() > 0){
            return true;
        }
    }
    return false;
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
    readers[orgRId]->addReader(shared->getId(), sharedRId);
    
    return true;
}

bool BaseFilter::setWriter(int writerID)
{
    if (writers.size() >= maxWriters) {
        utils::errorMsg("Too many writers!");
        return false;
    }
    
    if (writers.count(writerID) > 0){
        utils::warningMsg("Writer id must be unique");
        return false;
    }

    writers[writerID] = std::shared_ptr<Writer>(new Writer());
    seqNums[writerID] = 0;
    
    if (!specificWriterConfig(writerID)){
        writers.erase(writerID);
        seqNums.erase(writerID);
        return false;
    }
    
    return true;
}

bool BaseFilter::deleteWriter(int writerID)
{
    if (writers.count(writerID)){
        writers.erase(writerID);
        seqNums.erase(writerID);
        return specificWriterDelete(writerID);
    }
        
    return false;
}

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID)
{
    std::shared_ptr<Reader> r;
    FrameQueue *queue = NULL;
    ConnectionData cData;
    ReaderData reader;
    
    processEvent();
    R->processEvent();

    std::lock_guard<std::mutex> guard(mtx);
    
    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        utils::errorMsg("Reader " + std::to_string(readerID) + " null or already connected");
        return false;
    }
     
    if (!setWriter(writerID)){
        utils::warningMsg("Writer creation failed");
        return false;
    }

    cData.wFilterId = getId();
    cData.writerId = writerID;
    reader.rFilterId = R->getId();
    reader.readerId = readerID;
    cData.readers.push_back(reader);
    
    queue = allocQueue(cData);
    if (!queue){
        deleteWriter(writerID);
        return false;
    }

    if (!(r = R->setReader(readerID, queue))) {
        deleteWriter(writerID);
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
        return deleteWriter(writerId);
    }
    
    return false;
}

bool BaseFilter::disconnectReader(int readerId)
{   
    std::lock_guard<std::mutex> guard(mtx);
    
    if (readers.count(readerId) <= 0) {
        return false;
    }

    if (readers[readerId]->disconnect(getId())){
        return deleteReader(readerId);
    }

    return false;
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
    
    if (!demandOriginFrames(oFrames, newFrames) || !demandDestinationFrames(dFrames)){
        ret = WAIT;
        removeFrames(newFrames);
        return enabledJobs;
    }

    runDoProcessFrame(oFrames, dFrames, newFrames);
    
    //TODO: manage ret value
    enabledJobs = addFrames(dFrames);
    
    removeFrames(newFrames);

    return enabledJobs;
}

std::vector<int> BaseFilter::serverProcessFrame(int& ret)
{
    std::vector<int> enabledJobs;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
    std::vector<int> newFrames;
    
    processEvent();
    
    demandOriginFrames(oFrames, newFrames);
    demandDestinationFrames(dFrames);

    runDoProcessFrame(oFrames, dFrames, newFrames);

    enabledJobs = addFrames(dFrames);
    removeFrames(newFrames);
    
    ret = 0;
    
    return enabledJobs;
}

bool BaseFilter::demandOriginFrames(std::map<int, Frame*> &oFrames, std::vector<int> &newFrames)
{
    if (maxReaders == 0) {
        return true;
    }

    if (readers.empty()) {
        return false;
    }

    if (frameTime.count() <= 0 && readers.count(refReader) == 0) {
        return demandOriginFramesBestEffort(oFrames, newFrames);
    } else if (frameTime.count() <= 0 && readers.count(refReader) > 0) {
        return demandOriginFramesSync(oFrames, newFrames);
    } else {
        return demandOriginFramesFrameTime(oFrames, newFrames);
    }
}

//TODO: this is not functional test timestamps but not "get" the frame as then it is marked as consumed and it might not be the case.
bool BaseFilter::demandOriginFramesSync(std::map<int, Frame*> &oFrames, std::vector<int> &newFrames)
{      
    bool newFrame;
    Frame* frame;
    
    if (!readers[refReader] || !readers[refReader]->isConnected()) {
        deleteReader(refReader);
        return false;
    }
    
    frame = readers[refReader]->getFrame(getId(), newFrame);
    
    if (!frame) {
        utils::errorMsg("[BaseFilter::demandOriginFramesSync] Reader->getFrame() returned NULL. It cannot happen...");
        return false;
    }
    
    oFrames[refReader] = frame;
    frame->setConsumed(newFrame);
    if (newFrame){
        newFrames.push_back(refReader);
    }
    
    std::chrono::microseconds wallClock = frame->getPresentationTime();
    std::chrono::microseconds currentFTime;
        
    for (std::map<int, std::shared_ptr<Reader>>::iterator r = readers.begin() ; r != readers.end(); ) {
        if (!r->second || !r->second->isConnected()) {
            int rId = r->first;
            ++r;
            deleteReader(rId);
            continue;
        }
        
        if (r->first == refReader){
            ++r;
            continue;
        }
        
        frame = r->second->getFrame(getId(), newFrame);

        if (!frame) {
            utils::errorMsg("[BaseFilter::demandOriginFramesBestEffort] Reader->getFrame() returned NULL. It cannot happen...");
            ++r;
            continue;
        }
        
        currentFTime = frame->getPresentationTime();
        
        while (wallClock - syncMargin > currentFTime){
            r->second->removeFrame(getId());            
            frame = r->second->getFrame(getId(), newFrame);
            
            if (!frame) {
                utils::errorMsg("[BaseFilter::demandOriginFramesBestEffort] Reader->getFrame() returned NULL. It cannot happen...");
                return false;
            } else if (newFrame) {
                currentFTime = frame->getPresentationTime();
            } else {
                break;
            }
        }
        
        if (!(wallClock - syncMargin > currentFTime || wallClock + syncMargin < currentFTime)){
            oFrames[r->first] = frame;
            frame->setConsumed(newFrame);
            if (newFrame){
                newFrames.push_back(r->first);
            }
        }
        
        ++r;
    }
    
    if (oFrames.size() == readers.size()){
        return !newFrames.empty();;
    }

    return false;
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

bool BaseFilter::demandOriginFramesBestEffort(std::map<int, Frame*> &oFrames, std::vector<int> &newFrames) 
{
    bool newFrame;
    Frame* frame;
       
    for (std::map<int, std::shared_ptr<Reader>>::iterator r = readers.begin() ; r != readers.end(); ) {
        if (!r->second || !r->second->isConnected()) {
            int rId = r->first;
            ++r;
            deleteReader(rId);
            continue;
        }
        
        frame = r->second->getFrame(getId(), newFrame);

        if (!frame) {
            utils::errorMsg("[BaseFilter::demandOriginFramesBestEffort] Reader->getFrame() returned NULL. It cannot happen...");
            ++r;
            continue;
        }

        frame->setConsumed(newFrame);
        oFrames[r->first] = frame;
        if (newFrame){
            newFrames.push_back(r->first);
        }
        ++r;
    }

    return !newFrames.empty();
}

bool BaseFilter::demandOriginFramesFrameTime(std::map<int, Frame*> &oFrames, std::vector<int> &newFrames) 
{
    Frame* frame;
    std::chrono::microseconds outOfScopeTs = std::chrono::microseconds(-1);
    bool newFrame;
    bool validFrame = false;
    bool outDated = false;

    for (std::map<int, std::shared_ptr<Reader>>::iterator r = readers.begin() ; r != readers.end(); ) {
        if (!r->second || !r->second->isConnected()) {
            int rId = r->first;
            ++r;
            deleteReader(rId);
            continue;
        }

        frame = r->second->getFrame(getId(), newFrame);

        if (!frame) {
            utils::errorMsg("[BaseFilter::demandOriginFramesFrameTime] Reader->getFrame() returned NULL. It cannot happen...");
            ++r;
            continue;
        }

        frame->setConsumed(newFrame);
        oFrames[r->first] = frame;
        if (newFrame){
            newFrames.push_back(r->first);
        }

        if (!newFrame) {
            ++r;
            continue;
        }

        // If the current frame is out of our processing scope, 
        // we do not consider it as a new frame (keep noFrame value)
        if (frame->getPresentationTime() > syncTs + frameTime) {
            if (outOfScopeTs.count() < 0) {
                outOfScopeTs = frame->getPresentationTime();
            } else {
                outOfScopeTs = std::min(frame->getPresentationTime(), outOfScopeTs);
            }
            ++r;
            continue;
        }
        
        if (frame->getPresentationTime() < syncTs){
            outDated = true;
            ++r;
            continue;
        }

        // Normal case, which means that the frame is in our processing scope (syncTime -> syncTime+frameTime)
        validFrame = true;
        ++r;
    }

    // There is no new frame
    if (newFrames.empty()) {
        return false;
    }
    
    if (!validFrame) {
        if (outOfScopeTs > syncTs && !outDated) {
            syncTs = outOfScopeTs;
        }
        //We set framesToProcess true if there are no outDated frames
        return !outDated;
    } 

    // Finally set syncTs
    syncTs += frameTime;
    return true;
}

OneToOneFilter::OneToOneFilter(FilterRole fRole_, bool periodic) :
    BaseFilter(1, 1, fRole_, periodic)
{
}

bool OneToOneFilter::runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> /*newFrames*/)
{
    if (!doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        return false;
    }
    
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

