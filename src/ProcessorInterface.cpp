/*
 *  ProcessorInterface.cpp - Interface classes of frame processors
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
 *            
 */

#include "ProcessorInterface.hh"

//ProcessorInterface implementation

void ProcessorInterface::connect(MultiReader *R, MultiWriter *W)
{
    if (R == NULL || W == NULL){
        //TODO: error message
        return;
    }
    if (W->isConnectedById(R->getId())){
        //TODO: error message
        return;
    }
    if (!R->exceptMultiI() && R->connectedTo.size() >= 1){
        //TODO: error message
        return;
    }
    if (!W->exceptMultiO() && W->connectedTo.size() >= 1){
        //TODO: error message
        return;
    }
    FrameQueue *queue = W->allocQueue();
    std::pair<ProcessorInterface *, FrameQueue *> wConnection(R, queue);
    std::pair<ProcessorInterface *, FrameQueue *> rConnection(W, queue);
    W->connectedTo[R->getId()] = wConnection;
    R->connectedTo[W->getId()] = rConnection;
}

void ProcessorInterface::disconnect(ProcessorInterface *A, ProcessorInterface *B)
{
    if (A == NULL || B == NULL){
        //TODO: error message
        return;
    }
    if (!A->isConnectedById(B->getId())){
        //TODO: error message
        return;
    }
    A->connectedTo.erase (A->connectedTo.find(A->getId()));
    B->connectedTo.erase (B->connectedTo.find(B->getId()));
    //TODO: delete queue
}

ProcessorInterface::ProcessorInterface(ProcessorInterface *otherSide_): otherSide(otherSide_), id(currentId++)
{
}

bool ProcessorInterface::isConnected()
{
    return !connectedTo.empty();
}

bool ProcessorInterface::isConnectedById(int id_)
{
    ProcessorInterface *B;
    if (connectedTo.find(id_) != connectedTo.end()){
        if ((B = connectedTo.find(id_)->second.first) != NULL){
            return B->connectedTo.find(id) != B->connectedTo.end();
        }
    }
    return false;
}

void ProcessorInterface::setOtherSide(ProcessorInterface *otherSide_)
{
    otherSide = otherSide_;
}

//MultiReader implementation

MultiReader::MultiReader(MultiWriter *otherSide_) : ProcessorInterface(otherSide_)
{
}


void MultiReader::setQueueById(int id, FrameQueue *queue)
{
    if (isConnectedById(id) && connectedTo[id].second == NULL){
        connectedTo[id].second = queue;
    }
}


//SingleReader implementation

SingleReader::SingleReader(SingleWriter *otherSide_) : MultiReader(otherSide_)
{
}


void SingleReader::setQueue(FrameQueue *queue)
{
    int id;
    if (connectedTo.size() == 1){
        id = connectedTo.begin()->first;
        setQueueById(id, queue);
    } else {
        //TODO: error message
    }
}

FrameQueue* SingleReader::frameQueue(){
    if (connectedTo.size() == 1){
        return connectedTo.begin()->second.second;
    } else {
        //TODO: error message
        return NULL;
    }
}

//MultiWriter implementation

MultiWriter::MultiWriter(MultiReader *otherSide_) : ProcessorInterface(otherSide_)
{
}

bool MultiWriter::isQueueConnectedById(int id){
    ProcessorInterface *R;
    if (!isConnectedById(id)){
        return false;
    }
    if (connectedTo[id].second == NULL){
        return false;
    }
    R = connectedTo[id].first;
    if (connectedTo[id].second != R->getConnectedTo()[getId()].second){
        return false;
    }
    return true;
}

bool MultiWriter::connectQueueById(int id){
    if (!isConnectedById(id)){
        //TODO: error message
        return false;
    }
    FrameQueue *queue = allocQueue();
    if (queue == NULL){
        //TODO: error message
        return false;
    }
    if (MultiReader *R = dynamic_cast<MultiReader *> (connectedTo[id].first)) {
        connectedTo[id].second = queue;
        R->setQueueById(getId(), queue);
        return true;
    } else {
        return false;
    }
}

//SingleWriter implementation

SingleWriter::SingleWriter(SingleReader *otherSide_) : MultiWriter(otherSide_)
{
}

bool SingleWriter::isQueueConnected(){
    int id;
    if (connectedTo.size() == 1){
        id = connectedTo.begin()->first;
        return isQueueConnectedById(id);
    } else {
        //TODO: error message
        return false;
    }
}

bool SingleWriter::connectQueue(){
    int id;
    if (connectedTo.size() == 1){
        id = connectedTo.begin()->first;
        return connectQueueById(id);
    } else {
        //TODO: error message
        return false;
    }
}

FrameQueue* SingleWriter::frameQueue(){
    if (connectedTo.size() == 1){
        return connectedTo.begin()->second.second;
    } else {
        //TODO: error message
        return NULL;
    }
}

