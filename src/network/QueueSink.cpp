/*
 *  QueueSink.cpp - A Writer to the FrameQueue from liveMedia income
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors: David Cassany <david.cassany@i2cat.net> 
 *           Marc Palau <marc.palau@i2cat.net>
 */

#include "QueueSink.hh"
#include <iostream>
#include <sys/time.h>

QueueSink::QueueSink(UsageEnvironment& env, FrameQueue* queue)
  : MediaSink(env) 
{
    this->queue = queue;   
}

QueueSink* QueueSink::createNew(UsageEnvironment& env, FrameQueue* queue) 
{
    return new QueueSink(env, queue);
}

Boolean QueueSink::continuePlaying() 
{
    if (fSource == NULL) return False;

    //Check if there is any free slot in the queue
    updateFrame();

    fSource->getNextFrame(frame->getDataBuf(), frame->getMaxLength(),
              afterGettingFrame, this,
              onSourceClosure, this);

    return True;
}

void QueueSink::afterGettingFrame(void* clientData, unsigned frameSize,
                 unsigned numTruncatedBytes,
                 struct timeval presentationTime,
                 unsigned /*durationInMicroseconds*/) 
{
  QueueSink* sink = (QueueSink*)clientData;
  sink->afterGettingFrame(frameSize, presentationTime);
}

void QueueSink::afterGettingFrame(unsigned frameSize, struct timeval presentationTime) 
{
    frame->setLength(frameSize);
    frame->setUpdatedTime();
    frame->setPresentationTime(presentationTime);
    queue->addFrame();

    // Then try getting the next frame:
    continuePlaying();
}

void QueueSink::updateFrame(){
    while ((frame = queue->getRear()) == NULL) {
        std::cerr << "Queue overflow!" << std::endl;
        queue->flush();
    }
}