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

void ProcessorInterface::connect(ProcessorInterface *A, ProcessorInterface *B)
{
    if (A == NULL || B == NULL){
        //TODO: error message
        return;
    }
    if (A->isConnected() || B->isConnected()){
        //TODO: error message
        return;
    }
    A->connectedTo = B;
    B->connectedTo = A;
}

void ProcessorInterface::disconnect(ProcessorInterface *A, ProcessorInterface *B)
{
    if (A == NULL || B == NULL){
        //TODO: error message
        return;
    }
    if (!A->isConnected(B) || !B->isConnected(A)){
        //TODO: error message
        return;
    }
    A->connectedTo = B->connectedTo = NULL;
    //TODO: delete interfaces?
    //TODO: delete queue
}

ProcessorInterface::ProcessorInterface(ProcessorInterface *otherSide_): otherSide(otherSide_)
{
    connectedTo = NULL;
    queue = NULL;
}

bool ProcessorInterface::isConnected()
{
    return connectedTo != NULL;
}

bool ProcessorInterface::isConnected(ProcessorInterface *processor)
{
    return (connectedTo != NULL && connectedTo == processor);
}

ProcessorInterface* ProcessorInterface::getConnectedTo() const
{
    return connectedTo;
}

ProcessorInterface* ProcessorInterface::getOtherSide() const
{
    return otherSide;
}

void ProcessorInterface::setOtherSide(ProcessorInterface *otherSide_)
{
    otherSide = otherSide_;
}

FrameQueue* ProcessorInterface::frameQueue() const
{
    return queue;
}

//Reader implementation

Reader::Reader(Writer *otherSide_) : ProcessorInterface(otherSide_)
{
}

void Reader::receiveFrame(){
    if (frameQueue()->frameToRead()){
        toProcess();
    }
}

//Writer implementation

Writer::Writer(Reader *otherSide_) : ProcessorInterface(otherSide_)
{
}

void Writer::supplyFrame()
{
    if (Reader *connectedTo_ = dynamic_cast<Reader *> (connectedTo)){
        if (Reader *otherSide_ = dynamic_cast<Reader *> (otherSide)){
            connectedTo_->receiveFrame();
            if (otherSide_->frameQueue()->frameToRead()){
                otherSide_->toProcess();
            }
        }
    }
}

bool Writer::isQueueConnected(){
    if (queue == NULL){
        return false;
    }
    if (connectedTo->frameQueue() == NULL){
        return false;
    }
    if (queue != connectedTo->frameQueue()){
        return false;
    }
    return true;
}

bool Writer::connectQueue(){
    if (queue != NULL || connectedTo->frameQueue() != NULL) {
        //TODO: error message
        return false;
    }
    queue = allocQueue();
    if (queue == NULL){
        //TODO: error message
        return false;
    }
    if (Reader *connectedTo_ = dynamic_cast<Reader *> (connectedTo)) {
        connectedTo_->setQueue(queue);
        return true;
    } else {
        return false;
    }
}

//ReaderWriter implementation

ReaderWriter::ReaderWriter() : Reader(NULL), Writer(NULL)
{
    Reader *reader = this;
    Writer *writer = this;
    reader->setOtherSide(writer);
    writer->setOtherSide(reader);
}

void ReaderWriter::processFrame()
{
    Reader *reader = this;
    Writer *writer = this;
    
    bool newFrame = false;
    if ((org = reader->frameQueue()->getFront()) == NULL){
        toProcess();
        return;
    }
    if (!isQueueConnected() && !connectQueue()){
        //TODO: error message
        return;
    }
    dst = writer->frameQueue()->forceGetRear();
    newFrame = doProcessFrame(org, dst);
    reader->frameQueue()->removeFrame();
    if (newFrame){
        writer->frameQueue()->addFrame();
        supplyFrame();
    }
}

