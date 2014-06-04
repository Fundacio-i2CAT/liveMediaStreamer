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
#include <iostream>
#include <thread>
#include <chrono>

#define RETRIES 6
#define TIMEOUT 2500 //us

BaseFilter::BaseFilter(bool force_) : force(force_)
{
    readersCount = 0;
    writersCount = 0;
}

Reader* BaseFilter::getReader(int id) {
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}


Reader* setReader(int readerID, FrameQueue* queue)
{
    if (reader.count(id) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    return r;
}

bool BaseFilter::demandOriginFrames()
{
    bool newFrame = false;
    bool missedOne = false;
    for(int i = 0; i < RETRIES; i++){
        for (auto it : readers) {
            if (!it.second->isConnected()) {
                readers.erase(it);
                continue;
            }

            oFrames[it.first] = it.second->getFrame();
            if (oFrames[it.first] == NULL) {
                oFrames[it.first] = it.second->getFrame(force);
               // if (oFrames[it.first] == NULL) {
                    rUpdates[it.first] = false;
                    missedOne = true;
               // }
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

bool BaseFilter::connectOneToOne(BaseFilter *R)
{
    Reader r*;
    if (writers[DEFAULT_ID]->isConnected()) {
        return false;
    }

    if (R->getReader(DEFAULT_ID) && R->getReader(DEFAULT_ID)->isConnected()) {
        return false;
    }

    FrameQueue *queue = allocQueue(DEFAULT_ID);
    
    if (!(r = R->setReader(DEFAULT_ID, queue))) {
        return false;
    }
   // dFrames[wId] = NULL;
   // R->oFrames[rId] = NULL;
    writers[DEFAULT_ID]->setQueue(queue);
    return writers[DEFAULT_ID]->connect(r);
}

bool BaseFilter::connectManyToOne(BaseFilter *R, int wId)
{
    Reader r*;
    if (writers[wId]->isConnected()) {
        return false;
    }

    if (R->getReader(DEFAULT_ID) && R->getReader(DEFAULT_ID)->isConnected()){
        //TODO: delete QUEUE
        return false;
    }

    FrameQueue *queue = allocQueue(wId);
    
    if (!(r = R->setReader(DEFAULT_ID, queue)) {
        return false;
    }
   // dFrames[wId] = NULL;
   // R->oFrames[rId] = NULL;
    writers[wId]->setQueue(queue);
    return writers[wId]->connect(r);
}

bool connectOneToMany(BaseFilter *R, int rId)
{
    Reader r*;
    if (writers[DEFAULT_ID]->isConnected()) {
        return false;
    }

    if (R->getReader(rId) && R->getReader(rId)->isConnected()){
        //TODO: delete QUEUE
        return false;
    }

    FrameQueue *queue = allocQueue(DEFAULT_ID);
    
    if (!(r = R->setReader(rId, queue)) {
        return false;
    }
   // dFrames[wId] = NULL;
   // R->oFrames[rId] = NULL;
    writers[DEFAULT_ID]->setQueue(queue);
    return writers[DEFAULT_ID]->connect(r);
}


bool BaseFilter::connect(int wId, Reader *r)
{
    if (writers[wId]->isConnected()){
        return false;
    }
    if (r->isConnected()){
        return false;
    }
    FrameQueue *queue = allocQueue(wId);
    writers[wId]->setQueue(queue);
    return writers[wId]->connect(r);
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

OneToOneFilter::OneToOneFilter(bool force_) : 
BaseFilter(1, 1, force_)
{
}

bool OneToOneFilter::processFrame()
{
    bool newData = false;
    if (!demandOriginFrames() || !demandDestinationFrames()) {
        return false;
    }
    if (doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        addFrames();
    }
    removeFrames();
    return true;
}

OneToManyFilter::OneToManyFilter(int writersNum, bool force_) : 
BaseFilter(1, writersNum, force_)
{
}

bool OneToManyFilter::processFrame()
{
    bool newData;
    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }
    if (doProcessFrame(oFrames.begin()->second, dFrames)) {
        addFrames();
    }
    removeFrames();
    return true;
}

HeadFilter::HeadFilter(int writersNum) : 
BaseFilter()
{
    rwNextId = 0;
    for(int i = 0; i < writersNum; i++, ++rwNextId) {
        writers[rwNextId] = NULL;
    }
}

int HeadFilter::getNullWriterID()
{
    for (auto it : writers){
        if (it.second == NULL) {
            return it.first;
        }
    }

    return -1;
}


ManyToOneFilter::ManyToOneFilter(int readersNum, bool force_) : 
BaseFilter(readersNum, 1, force_)
{
}

bool ManyToOneFilter::processFrame()
{
    bool newData;
    
    if (!demandOriginFrames() || !demandDestinationFrames()) {
        return false;
    }

    if (doProcessFrame(oFrames, dFrames.begin()->second)) {
        addFrames();
    }

    removeFrames();
    
    return true;
}
