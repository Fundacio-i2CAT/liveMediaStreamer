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

Reader::Reader(bool sharedQueue)
{
    this->sharedQueue = sharedQueue;
    queue = NULL;
}

Reader::~Reader()
{
    disconnect();
}

void Reader::setQueue(FrameQueue *queue)
{
    this->queue = queue;
}

QueueState Reader::getFrame(Frame* frame, bool &newFrame, bool force)
{
    if (!queue->isConnected()) {
        utils::errorMsg("The queue is not connected");
        newFrame = false;
        frame = NULL;
        return queue->getState();
    }

    frame = queue->getFront(newFrame);

    if (force && frame == NULL) {
        frame = queue->forceGetFront(newFrame);
    }

    return queue->getState();
}

void Reader::removeFrame() 
{
    queue->removeFrame();
}

void Reader::setConnection(FrameQueue *queue)
{
    this->queue = queue;
}

void Reader::disconnect()
{
    if (!queue) {
        return;
    }

    if (sharedQueue) {
        queue = NULL;
        return;
    }

    if (queue->isConnected()) {
        queue->setConnected(false);
        queue = NULL;
    } else {
        delete queue;
        queue = NULL;
    }
}

bool Reader::isConnected()
{
    if (!queue) {
        return false;
    }

    return queue->isConnected();
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

bool Writer::connect(Reader *reader)
{
    if (!queue) {
        //TODO: error msg
		utils::errorMsg("The queue is empty");
        return false;
    }

    reader->setConnection(queue);
    queue->setConnected(true);
    return true;
}

void Writer::disconnect()
{
    if (!queue) {
        return;
    }

    if (queue->isConnected()) {
        queue->setConnected(false);
        queue = NULL;
    } else {
        delete queue;
        queue = NULL;
    }
}

void Writer::disconnect(Reader *reader)
{
    reader->disconnect();
    disconnect();
}

bool Writer::isConnected()
{
    if (!queue) {
        return false;
    }

    return queue->isConnected();
}

void Writer::setQueue(FrameQueue *queue)
{
    this->queue = queue;
}

Frame* Writer::getFrame(bool force)
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

void Writer::addFrame()
{
    queue->addFrame();
}

