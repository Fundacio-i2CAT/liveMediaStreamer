/*
 *  IOInterface.cpp - Interface classes of frame processors
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

#include "IOInterface.hh"
#include "Utils.hh"

/////////////////////////
//READER IMPLEMENTATION//
/////////////////////////

Reader::Reader(std::chrono::microseconds wDelay) : queue(NULL), frame(NULL), ready(true),
                    avgDelay(std::chrono::microseconds(0)), delay(std::chrono::microseconds(0)), windowDelay(wDelay), 
                    lastTs(std::chrono::microseconds(-1)), timeCounter(std::chrono::microseconds(0)), frameCounter(0)
{
}

Reader::~Reader()
{
    disconnectQueue();
}

bool Reader::disconnectQueue()
{
    if (!queue) {
        return false;
    }

    if (queue->isConnected()) {
        queue->setConnected(false);
    } else {
        delete queue;
    }
    
    queue = NULL;
    
    return true;
}

void Reader::addReader(int fId, int rId)
{
    std::lock_guard<std::mutex> guard(lck);
        
    if (queue->isConnected() && queue->addReaderCData(fId, rId)){
        filters[fId].first = false;
        filters[fId].second = false;
    }
}

void Reader::removeReader(int id)
{
    std::unique_lock<std::mutex> guard(lck);
    
    if (queue){
        queue->removeReaderCData(id);
    }
    
    if (filters.count(id) > 0){
        filters.erase(id);
    }
    
    if (filters.size() == 0){
        guard.unlock();
        disconnectQueue();
    }
}

Frame* Reader::getFrame(int fId, bool &newFrame)
{
    std::lock_guard<std::mutex> guard(lck);
    
    if (!queue->isConnected()) {
        utils::errorMsg("The queue is not connected");
        return NULL;
    }
    
    if (filters.count(fId) == 0) {
        utils::errorMsg("Reader not included in filter: " + std::to_string(fId));
        return NULL;
    }

    if (!frame) {
        frame = queue->getFront();
    }

    if (!frame) {
        newFrame = false;
        return queue->forceGetFront();
    }

    if (ready){
        for (auto& f : filters){
            f.second.first = true;
            f.second.second = true;
        }
        ready = false;
    }
    
    if (filters[fId].first){
        newFrame = true;
        filters[fId].first = false;
    } else {
        newFrame = false;
    }

    return frame;
}

bool Reader::allRead()
{  
    for (auto it : filters){
        if (it.second.first || it.second.second){
            return false;
        }
    }

    return true;
}

int Reader::removeFrame(int fId)
{
    std::lock_guard<std::mutex> guard(lck);
    
    if (filters.count(fId) > 0){
        filters[fId].second = false;
    }
    
    if (allRead()){
        measureDelay();
        frame = NULL;
        ready = true;
        return queue->removeFrame();
    }
    
    return -1;
}

void Reader::measureDelay()
{
    if(lastTs.count() < 0){
        lastTs = frame->getPresentationTime();
    }

    if (lastTs == frame->getPresentationTime()) {
        return;
    }

    timeCounter += frame->getPresentationTime() - lastTs;
    lastTs = frame->getPresentationTime();

    if(timeCounter >= windowDelay && frameCounter > 0){
        avgDelay = delay / frameCounter;
        timeCounter = std::chrono::microseconds(0);
        delay = std::chrono::microseconds(0);
        frameCounter = 0;
    }

    delay += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - frame->getOriginTime());
    frameCounter++;
}

std::chrono::microseconds Reader::getAvgDelay()
{ 
    std::lock_guard<std::mutex> guard(lck);

    return avgDelay; 
};

size_t Reader::getLostBlocs()
{ 
    std::lock_guard<std::mutex> guard(lck);

    return queue->getLostBlocs(); 
};

void Reader::setConnection(FrameQueue *queue)
{
    if (isConnected() || !queue){
        return;
    }
    
    std::lock_guard<std::mutex> guard(lck);
    ReaderData rData;
    
    this->queue = queue;
    
    rData = queue->getCData().readers.front();
    filters[rData.rFilterId].first = false;
    filters[rData.rFilterId].second = false;
}

bool Reader::disconnect(int id)
{
    std::lock_guard<std::mutex> guard(lck);
    
    if (filters.count(id) == 0){
        return false;
    }
    
    filters.erase(id);
    if (queue){
        queue->removeReaderCData(id);
    }
    
    if (filters.size() > 1){
        return true;
    }
    
    return disconnectQueue();
}

bool Reader::isConnected()
{
    if (!queue) {
        return false;
    }

    return queue->isConnected();
}

size_t Reader::getQueueElements()
{
    if (!queue) {
        return 0;
    }

    return queue->getElements();
}

std::chrono::microseconds Reader::getCurrentTime()
{
    if (frame){
        return frame->getPresentationTime();
    }
    
    Frame *f;
    
    f = queue->getFront();
    if (f){
        return f->getPresentationTime();
    }
    
    return std::chrono::microseconds(0);
}

bool Reader::isFull() const
{
    return queue->isFull(); 
}



/////////////////////////
//WRITER IMPLEMENTATION//
/////////////////////////

Writer::Writer()
{
    queue = NULL;
}

Writer::~Writer()
{
    disconnect();
}

bool Writer::connect(std::shared_ptr<Reader> reader) const
{
    if (!queue) {
        utils::errorMsg("The queue is NULL");
        return false;
    }

    reader->setConnection(queue);    
    queue->setConnected(true);
    return true;
}

bool Writer::disconnect() const
{
    if (!queue) {
        return false;
    }

    if (queue->isConnected()) {
        queue->setConnected(false);
        queue = NULL;
    } else {
        delete queue;
        queue = NULL;
    }
    return true;
}

bool Writer::disconnect(std::shared_ptr<Reader> reader) const
{
    //NOTE: Maybe it should disconnect all associated consumers too
    return disconnect();
}

bool Writer::isConnected() const
{
    if (!queue) {
        return false;
    }

    return queue->isConnected();
}

void Writer::setQueue(FrameQueue *queue) const
{
    this->queue = queue;
}

Frame* Writer::getFrame(bool force) const
{
    Frame* frame;

    if (!queue->isConnected()) {
        utils::errorMsg("The queue is not connected");
        return NULL;
    }

    frame = queue->getRear();

    if (frame == NULL && force) {
        frame = queue->forceGetRear();
    }

    return frame;
}

std::vector<int> Writer::addFrame() const
{
    return queue->addFrame();
}

ConnectionData Writer::getCData()
{
    if (queue && queue->isConnected()){
        return queue->getCData();
    }
    
    ConnectionData cData;
    return cData;
    
}
