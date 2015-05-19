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

BaseFilter::BaseFilter(unsigned readersNum, unsigned writersNum, size_t fTime, FilterRole fRole_, bool force_, bool sharedFrames_):
process(false), maxReaders(readersNum), maxWriters(writersNum),  frameTime(fTime), fRole(fRole_), force(force_), sharedFrames(sharedFrames_)
{
    frameTimeMod = 1;
    bufferStateFrameTimeMod = 1;
    timestamp = std::chrono::system_clock::now();
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

void BaseFilter::setFrameTime(std::chrono::nanoseconds fTime)
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

bool BaseFilter::demandOriginFrames()
{
    bool newFrame;
    bool someFrame = false;
    QueueState qState;

    if (maxReaders == 0) {
        return true;
    }

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
    std::lock_guard<std::mutex> guard(eventQueueMutex);

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

bool BaseFilter::hasFrames()
{
	if (!demandOriginFrames() || !demandDestinationFrames()) {
		return false;
	}

	return true;
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

std::chrono::nanoseconds BaseFilter::processFrame()
{
    std::chrono::nanoseconds retValue;

    switch(fRole) {
        case MASTER:
            retValue = masterProcessFrame();
            break;
        case SLAVE:
            retValue = slaveProcessFrame();
            break;
        case NETWORK:
            runDoProcessFrame();
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
    size_t frameTime_value;
    
    wallClock = std::chrono::system_clock::now();

    processEvent();
      
    if (!demandOriginFrames() || !demandDestinationFrames()) {
        return std::chrono::nanoseconds(RETRY);
    }
    
    processAll();
    
    runDoProcessFrame();

    while (runningSlaves()){
        std::this_thread::sleep_for(std::chrono::nanoseconds(RETRY));
    }

    removeFrames();

    if (frameTime.count() == 0){
        return std::chrono::nanoseconds(RETRY);
    }

    enlapsedTime = (std::chrono::duration_cast<std::chrono::nanoseconds>
        (std::chrono::system_clock::now() - wallClock));

    frameTime_value = frameTime.count()*frameTimeMod*bufferStateFrameTimeMod;

    frameTime_ = std::chrono::nanoseconds(frameTime_value);
    
    if (enlapsedTime > frameTime_){
        return std::chrono::nanoseconds(0);
    }

    return frameTime_ - enlapsedTime;
}

std::chrono::nanoseconds BaseFilter::slaveProcessFrame()
{
    if (!process) {
        return std::chrono::nanoseconds(RETRY);
    }

    processEvent();

    //TODO: decide policy to set run to true/false if retry
    if (!demandDestinationFrames()){
        return std::chrono::nanoseconds(RETRY);
    }

    if (!sharedFrames && !demandOriginFrames()) {
        return std::chrono::nanoseconds(RETRY);
    }

    runDoProcessFrame();

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

OneToOneFilter::OneToOneFilter(bool byPassTimestamp, FilterRole fRole_, bool sharedFrames_, size_t fTime, bool force_) :
    BaseFilter(1,1,fTime,fRole_,force_,sharedFrames_), passTimestamp(byPassTimestamp)
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
        addFrames();
        return true;
    }

    return false;
}


OneToManyFilter::OneToManyFilter(FilterRole fRole_, bool sharedFrames_, unsigned writersNum, size_t fTime, bool force_) :
    BaseFilter(1,writersNum,fTime,fRole_,force_,sharedFrames_)
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

        addFrames();
        return true;
    }

    return false;
}


HeadFilter::HeadFilter(FilterRole fRole_, size_t fTime) :
    BaseFilter(0, 1, fTime, fRole_, false, false)
{
    
}

bool HeadFilter::runDoProcessFrame()
{
    if (updateTimestamp() && doProcessFrame(dFrames.begin()->second)) {
        dFrames.begin()->second->setPresentationTime(timestamp);
        dFrames.begin()->second->setDuration(duration);
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

    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}

TailFilter::TailFilter(FilterRole fRole_, bool sharedFrames_, unsigned readersNum, size_t fTime) :
    BaseFilter(readersNum, 0, fTime, fRole_, false, sharedFrames_)
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

    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}


ManyToOneFilter::ManyToOneFilter(FilterRole fRole_, bool sharedFrames_, unsigned readersNum, size_t fTime, bool force_) :
    BaseFilter(readersNum,1,fTime,fRole_,force_,sharedFrames_)
{
}

bool ManyToOneFilter::runDoProcessFrame()
{
    if (updateTimestamp() && doProcessFrame(oFrames, dFrames.begin()->second)) {
        dFrames.begin()->second->setPresentationTime(timestamp);
        dFrames.begin()->second->setDuration(duration);
        seqNums[dFrames.begin()->first]++;
        dFrames.begin()->second->setSequenceNumber(seqNums[dFrames.begin()->first]);
        addFrames();
        return true;
    }

    return false;
}

LiveMediaFilter::LiveMediaFilter(unsigned readersNum, unsigned writersNum) :
BaseFilter(readersNum, writersNum, 0, NETWORK, false, false)
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

    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}
