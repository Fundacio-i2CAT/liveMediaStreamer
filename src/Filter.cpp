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

#define WALL_CLOCK_THRESHOLD 2.5 //number of frames
#define SLOW_MODIFIER 1.10
#define FAST_MODIFIER 0.90

BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, size_t fTime, FilterRole fRole_, bool force_, bool periodic): Runnable(periodic), process(false), 
maxReaders(readersNum), maxWriters(writersNum),  frameTime(fTime), fRole(fRole_), force(force_)
{
    frameTimeMod = 1;
    bufferStateFrameTimeMod = 1;
    timestamp = std::chrono::system_clock::now();
}

BaseFilter::~BaseFilter()
{
    std::lock_guard<std::mutex> guard(readersWritersLck);
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
}

void BaseFilter::setFrameTime(std::chrono::nanoseconds fTime)
{
    frameTime = fTime;
}

Reader* BaseFilter::getReader(int id)
{
    std::lock_guard<std::mutex> guard(readersWritersLck);
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}

Reader* BaseFilter::setReader(int readerID, FrameQueue* queue)
{
    std::lock_guard<std::mutex> guard(readersWritersLck);
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    return r;
}

int BaseFilter::generateReaderID()
{
    std::lock_guard<std::mutex> guard(readersWritersLck);
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
    std::lock_guard<std::mutex> guard(readersWritersLck);
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

    std::lock_guard<std::mutex> guard(readersWritersLck);
    
    if (maxReaders == 0) {
        return true;
    }

    for (auto it : readers) {
        if (!it.second->isConnected()) {
            it.second->disconnect();
            //NOTE: think about readers and queues as shared pointers
            delete it.second;
            readers.erase(it.first);
            continue;
        }

        oFrames[it.first] = it.second->getFrame(qState, newFrame, force);

        if (oFrames[it.first] != NULL) {
            if (newFrame) {
                oFrames[it.first]->setConsumed(true);
            } else {
                oFrames[it.first]->setConsumed(false);
            }

            someFrame = true;
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
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
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
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
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
    
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
    for (auto it : readers){
        if (oFrames[it.first] && oFrames[it.first]->getConsumed()){
            enabledJobs.push_back(it.second->removeFrame());
        }
    }
    
    return enabledJobs;
}

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID)
{
    Reader* r;
    FrameQueue *queue = NULL;
    
    std::lock_guard<std::mutex> guard(readersWritersLck);
      
    if (writers.size() < maxWriters && writers.count(writerID) <= 0) {
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

    queue = allocQueue(getId(), R->getId(), writerID);
    if (!queue){
        return false;
    }

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
    
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
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
    
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
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
    std::lock_guard<std::mutex> guard(readersWritersLck);
    
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
    doGetState(filterNode);
}

bool BaseFilter::updateTimestamp()
{   
    int threshold = frameTime.count() * WALL_CLOCK_THRESHOLD;
    
    if (frameTime.count() == 0) {
        lastValidTimestamp = timestamp;
        timestamp = wallClock;
        duration = timestamp - lastValidTimestamp;
        return true;
    }

    timestamp += frameTime;

    lastDiffTime = diffTime;
    diffTime = wallClock - timestamp;

    if (diffTime.count() > threshold || diffTime.count() < (-threshold) ) {
        // reset timestamp value in order to realign with the wall clock
        utils::warningMsg("Wall clock deviations exceeded! Reseting values!");
        timestamp = wallClock;
        duration = frameTime + diffTime;
        diffTime = std::chrono::nanoseconds(0);
        frameTimeMod = 1;
    } else {
        lastValidTimestamp = timestamp;
        duration = frameTime;
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
    
    return timestamp >= lastValidTimestamp;
}

std::vector<int> BaseFilter::processFrame(int& ret)
{
    std::vector<int> enabledJobs;

    switch(fRole) {
        case MASTER:
            enabledJobs = masterProcessFrame(ret);
            break;
        case SLAVE:
            enabledJobs = slaveProcessFrame(ret);
            break;
        case NETWORK:
            runDoProcessFrame();
            ret = 0;
            break;
        case SERVER:
            enabledJobs = serverProcessFrame(ret);
        default:
            ret = 0;
            break;
    }

    return enabledJobs;
}

void BaseFilter::processAll()
{
    for (auto it : slaves) {
        it.second->updateFrames(oFrames);
        it.second->setWallClock(wallClock);
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

bool BaseFilter::addSlave(BaseFilter *slave)
{
    if (slave->fRole != SLAVE){
        utils::warningMsg("Could not add slave, invalid role");
        return false;
    }
    
    if (slave->getId() < 0){
        utils::warningMsg("Could not add slave, invalid id");
        return false;
    }

    if (slaves.count(slave->getId()) > 0){
        utils::warningMsg("Could not add slave, id must be unique");
        return false;
    }

    slaves[slave->getId()] = slave;

    return true;
}

std::vector<int> BaseFilter::masterProcessFrame(int& ret)
{
    std::chrono::nanoseconds enlapsedTime;
    std::chrono::nanoseconds frameTime_;
    size_t frameTime_value;
    std::vector<int> enabledJobs;
    
    wallClock = std::chrono::system_clock::now();

    processEvent();
      
    if (!demandOriginFrames()) {
        ret = RETRY/1000;
        return enabledJobs;
    }
    
    if (!demandDestinationFrames()) {
        ret = RETRY/1000;
        return enabledJobs;
    }

    processAll();

    runDoProcessFrame();

    while (runningSlaves()){
        std::this_thread::sleep_for(std::chrono::nanoseconds(RETRY));
    }

    enabledJobs = addFrames();
    removeFrames();

    if (frameTime.count() == 0) {
        ret = 0;
        return enabledJobs;
    }

    enlapsedTime = (std::chrono::duration_cast<std::chrono::nanoseconds>
        (std::chrono::system_clock::now() - wallClock));

    frameTime_value = frameTime.count()*frameTimeMod*bufferStateFrameTimeMod;

    frameTime_ = std::chrono::nanoseconds(frameTime_value);
    
    if (enlapsedTime > frameTime_){
        utils::warningMsg("Your computer is too slow");
        ret = 0;
        return enabledJobs;
    }

    ret = (std::chrono::duration_cast<std::chrono::microseconds>(frameTime_ - enlapsedTime)).count();
    
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

std::vector<int> BaseFilter::slaveProcessFrame(int& ret)
{
    std::vector<int> enabledJobs;
    ret = RETRY/1000;
    
    if (!process) {
        ret = RETRY/1000;
        return enabledJobs;
    }

    processEvent();

    //TODO: decide policy to set run to true/false if retry
    if (!demandDestinationFrames()){
        return enabledJobs;;
    }

    if (!demandOriginFrames()) {
        return enabledJobs;
    }

    runDoProcessFrame();
    
    enabledJobs = addFrames();

    process = false;
    return enabledJobs;
}

void BaseFilter::updateFrames(std::map<int, Frame*> oFrames_)
{
    oFrames = oFrames_;
}

OneToOneFilter::OneToOneFilter(bool byPassTimestamp, FilterRole fRole_, size_t fTime, bool force_, bool periodic) :
    BaseFilter(1,1,fTime,fRole_,force_, periodic), passTimestamp(byPassTimestamp)
{
}

bool OneToOneFilter::runDoProcessFrame()
{
    if (updateTimestamp() && doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        if (passTimestamp){
            dFrames.begin()->second->setPresentationTime(oFrames.begin()->second->getPresentationTime());
        } else {
            dFrames.begin()->second->setPresentationTime(timestamp);
        }
        dFrames.begin()->second->setDuration(duration);
        dFrames.begin()->second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
        return true;
    }

    return false;
}


OneToManyFilter::OneToManyFilter(FilterRole fRole_, unsigned writersNum, size_t fTime, bool force_, bool periodic) :
    BaseFilter(1,writersNum,fTime,fRole_,force_, periodic)
{
}

bool OneToManyFilter::runDoProcessFrame()
{
    if (updateTimestamp() && doProcessFrame(oFrames.begin()->second, dFrames)) {
        for (auto it : dFrames) {
            it.second->setPresentationTime(timestamp);
            it.second->setDuration(duration);
            it.second->setSequenceNumber(oFrames.begin()->second->getSequenceNumber());
        }
        return true;
    }

    return false;
}


HeadFilter::HeadFilter(FilterRole fRole_, size_t fTime, unsigned writersNum, bool periodic) :
    BaseFilter(0, writersNum, fTime, fRole_, false, periodic)
{
}

bool HeadFilter::runDoProcessFrame()
{
    return doProcessFrame(dFrames);
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

TailFilter::TailFilter(FilterRole fRole_, unsigned readersNum, size_t fTime, bool periodic) :
    BaseFilter(readersNum, 0, fTime, fRole_, false, periodic)
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


ManyToOneFilter::ManyToOneFilter(FilterRole fRole_, unsigned readersNum, size_t fTime, bool force_, bool periodic) :
    BaseFilter(readersNum,1,fTime,fRole_,force_, periodic)
{
}

bool ManyToOneFilter::runDoProcessFrame()
{
    if (updateTimestamp() && doProcessFrame(oFrames, dFrames.begin()->second)) {
        dFrames.begin()->second->setPresentationTime(timestamp);
        dFrames.begin()->second->setDuration(duration);
        seqNums[dFrames.begin()->first]++;
        dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]);
        return true;
    }

    return false;
}

LiveMediaFilter::LiveMediaFilter(unsigned readersNum, unsigned writersNum, bool periodic) :
BaseFilter(readersNum, writersNum, 0, NETWORK, false, periodic)
{
}

bool LiveMediaFilter::runDoProcessFrame()
{
    return doProcessFrame();
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
