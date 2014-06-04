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

/////////////////////////
//READER IMPLEMENTATION//
/////////////////////////

Reader::Reader()
{
    queue = NULL;
    connected = false;
}

Reader::~Reader()
{
    delete queue;
}

void Reader::setQueue(FrameQueue *queue)
{
    this->queue = queue;
}

Frame* Reader::getFrame(bool force)
{
    Frame* frame;

    if (!connected) {
        //TODO: error msg
        return NULL;
    }

    frame = queue->getFront();

    if (frame == NULL && force) {
        frame = queue->forceGetFront();
    }

    return frame;
}

void Reader::removeFrame() 
{
    queue->removeFrame();
}

void Reader::setConnection(FrameQueue *queue)
{
    this->queue = queue;
    connected = true;
}

void Reader::disconnect()
{
    connected = false;
}

/////////////////////////
//WRITER IMPLEMENTATION//
/////////////////////////

Writer::Writer()
{
    queue = NULL;
    connected = false;
}

bool Writer::connect(Reader *reader)
{
    if (!queue) {
        //TODO: error msg
        return false;
    }

    reader->setConnection(queue);

    connected = true;
    return true;
}

void Writer::disconnect(Reader *reader)
{
    reader->disconnect();

    queue = NULL;
    connected = false;
}

void Writer::setQueue(FrameQueue *queue)
{
    this->queue = queue;
}

Frame* Writer::getFrame(bool force)
{
    Frame* frame;

    if (!connected) {
        //TODO: error msg
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

