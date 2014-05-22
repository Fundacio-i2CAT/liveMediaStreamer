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

BaseFilter::BaseFilter(int readersNum, int writersNum, bool force_) : force(force_)
{
    rwNextId = 0;
    for(int i = 0; i < readersNum; i++, ++rwNextId){
        readers[rwNextId] = new Reader();
        rUpdates[rwNextId] = false;
        oFrames[rwNextId] = NULL;
    }
    for(int i = 0; i < writersNum; i++, ++rwNextId){
        writers[rwNextId] = new Writer();
        dFrames[rwNextId] = NULL;
    }
}

Reader* BaseFilter::getReader(int id){
    if (readers.find(id) != readers.end()){
        return readers.find(id)->second;
    }
    return NULL;
}

bool BaseFilter::demandOriginFrames()
{
    bool newFrame = false;
    for (auto it : readers){
        if (it.second->isConnected()){
            oFrames[it.first] = it.second->getFrame(force);
            if (oFrames[it.first] == NULL){
                rUpdates[it.first] = false; 
            } else {
                rUpdates[it.first] = true;
                newFrame = true;
            }
        } else {
            rUpdates[it.first] = false;
            oFrames[it.first] = NULL;
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
        } else {
            dFrames[it.first] = NULL;
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

bool BaseFilter::connect(int wId, BaseFilter *R, int rId)
{
    if (writers[wId]->isConnected()){
        return false;
    }
    Reader *r = R->getReader(rId);
    if (r->isConnected()){
        return false;
    }
    FrameQueue *queue = allocQueue();
    writers[wId]->setQueue(queue);
    return writers[wId]->connect(r);
}

bool BaseFilter::disconnect(int wId, BaseFilter *R, int rId)
{
    if (! writers[wId]->isConnected()){
        return false;
    }
    Reader *r = R->getReader(rId);
    if (! r->isConnected()){
        return false;
    }
    writers[wId]->disconnect(r);
    return true;
}

std::vector<int> BaseFilter::getAvailableReaders()
{
    std::vector<int> readersId;
    for (auto it : readers){
        if (!it.second->isConnected()) {
            readersId.push_back(it.first);
        }
    }
    return readersId;
}

std::vector<int> BaseFilter::getAvailableWriters()
{
    std::vector<int> writersId;
    for (auto it : writers){
        if (!it.second->isConnected()) {
            writersId.push_back(it.first);
        }
    }
    return writersId;
}

OneToOneFilter::OneToOneFilter(bool force_) : 
BaseFilter(1, 1, force_)
{
}

bool OneToOneFilter::processFrame()
{
    bool newData = false;
    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }
    if (doProcessFrame(oFrames.begin()->second, 
        dFrames.begin()->second)){
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
    if (doProcessFrame(oFrames.begin()->second, 
        dFrames)){
        addFrames();
        }
        removeFrames();
    return true;
}

ManyToOneFilter::ManyToOneFilter(int readersNum, bool force_) : 
BaseFilter(readersNum, 1, force_)
{
}

bool ManyToOneFilter::processFrame()
{
    bool newData;
    if (!demandOriginFrames() || !demandDestinationFrames()){
        return false;
    }
    if (doProcessFrame(oFrames, 
        dFrames.begin()->second)){
        addFrames();
        }
        removeFrames();
    return true;
}
