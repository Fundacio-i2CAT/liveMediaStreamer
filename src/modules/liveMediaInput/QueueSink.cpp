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
#include "../../Utils.hh"

#include <sys/time.h>

QueueSink::QueueSink(UsageEnvironment& env, unsigned port)
  : MediaSink(env), fPort(port), nextFrame(true)
{
    frame = NULL;
    dummyBuffer = new unsigned char[DUMMY_RECEIVE_BUFFER_SIZE];
}

QueueSink::~QueueSink()
{
    delete[] dummyBuffer;
}

QueueSink* QueueSink::createNew(UsageEnvironment& env, unsigned port)
{
    return new QueueSink(env, port);
}

Boolean QueueSink::continuePlaying()
{
    if (fSource == NULL) {
        utils::errorMsg("Cannot play, fSource is null");
        return False;
    }

    if (!frame){
        utils::debugMsg("Using dummy buffer, no writer connected yet");
        fSource->getNextFrame(dummyBuffer, DUMMY_RECEIVE_BUFFER_SIZE,
                              afterGettingFrame, this,
                              onSourceClosure, this);
        return True;
    }
    
    if (nextFrame){
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)QueueSink::staticContinuePlaying, this);
        return True;
    }


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
    std::chrono::microseconds ts = std::chrono::microseconds(presentationTime.tv_sec * std::micro::den + presentationTime.tv_usec);

    if (frame != NULL) {
        frame->setLength(frameSize);
        frame->newOriginTime();
        frame->setPresentationTime(ts);
        frame->setConsumed(true);
        nextFrame = true;
    }
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)QueueSink::staticContinuePlaying, this);
}

void QueueSink::staticContinuePlaying(QueueSink *sink)
{
    sink->continuePlaying();
}

bool QueueSink::setFrame(Frame *f){
    if (nextFrame){
        frame = f;
        nextFrame = false;
        return true;
    }
    return false;
}