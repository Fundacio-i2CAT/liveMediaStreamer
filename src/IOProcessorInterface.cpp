/*
 *  IOProcessorInterface.hh - Input and Ouput interfaces for frame processors
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

#include "IOProcessorInterface.hh"

//MultiReaderSingleWriter implementation

MultiReaderSingleWriter::MultiReaderSingleWriter() : MultiReader(NULL), SingleWriter(NULL)
{
    MultiReader *reader = this;
    SingleWriter *writer = this;
    reader->setOtherSide(writer);
    writer->setOtherSide(reader);
}

bool MultiReaderSingleWriter::demandFrames()
{
    MultiReader *reader = this;
    SingleWriter *writer = this;
    std::map<int, std::pair<ProcessorInterface*, FrameQueue*>> connections;
    FrameQueue *queue;
    Frame *frame;
    int id;
    bool newFrame = false;
    
    if (!isQueueConnected() && !connectQueue()){
        //TODO: log error message
        return false;
    }
    
    connections = reader->getConnectedTo();
    for (auto it : connections){
        id = it.first;
        queue = it.second.second;
        if ((frame = queue->getFront()) == NULL){
            updates[id] = false;
            frame = queue->forceGetFront();
            orgFrames[id] = frame;
        } else {
            updates[id] = true;
            orgFrames[id] = frame;
            newFrame = true;
        }
    }
    
    dst = writer->frameQueue()->forceGetRear();
    return newFrame;
}

void MultiReaderSingleWriter::supplyFrame(bool newFrame)
{
    MultiReader *reader = this;
    SingleWriter *writer = this;
    std::map<int, std::pair<ProcessorInterface*, FrameQueue*>> connections;
    int id;
    
    connections = reader->getConnectedTo();
    for (auto it : updates){
        id = it.first;
        connections[id].second->removeFrame();
    }
    
    if (newFrame){
        writer->frameQueue()->addFrame();
    }
}

bool MultiReaderSingleWriter::processFrame()
{
    bool newFrame;
    if (!demandFrames()){
        return false;
    }
    newFrame = doProcessFrame(orgFrames, dst);
    supplyFrame(newFrame);
    return true;
}



//SingleReaderSingleWriter implementation

SingleReaderSingleWriter::SingleReaderSingleWriter() : SingleReader(NULL), SingleWriter(NULL)
{
    SingleReader *reader = this;
    SingleWriter *writer = this;
    reader->setOtherSide(writer);
    writer->setOtherSide(reader);
}

bool SingleReaderSingleWriter::demandFrame()
{
    SingleReader *reader = this;
    SingleWriter *writer = this;
    if ((org = reader->frameQueue()->getFront()) == NULL){
        //TODO: log error message
        return false;
    }
    if (!isQueueConnected() && !connectQueue()){
        //TODO: log error message
        return false;
    }
    dst = writer->frameQueue()->forceGetRear();
    return true;
}

void SingleReaderSingleWriter::supplyFrame(bool newFrame)
{
    SingleReader *reader = this;
    SingleWriter *writer = this;
    reader->frameQueue()->removeFrame();
    if (newFrame){
        writer->frameQueue()->addFrame();
    }
}

bool SingleReaderSingleWriter::processFrame()
{
    bool newFrame;
    if (!demandFrame()){
        return false;
    }
    newFrame = doProcessFrame(org, dst);
    supplyFrame(newFrame);
    return true;
}


