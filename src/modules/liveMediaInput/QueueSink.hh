/*
 *  QueueSink.hh - A Writer to the FrameQueue from liveMedia income
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

#ifndef _QUEUE_SINK_HH
#define _QUEUE_SINK_HH

#include <liveMedia.hh>

#include "../../Frame.hh"
#include "../../IOInterface.hh"

#define DUMMY_RECEIVE_BUFFER_SIZE 200000

class QueueSink: public MediaSink {

public:
    static QueueSink* createNew(UsageEnvironment& env, Writer *writer, unsigned port);
    Writer* getWriter() const {return fWriter;};
    unsigned getPort() {return fPort;};

protected:
    QueueSink(UsageEnvironment& env, Writer *writer, unsigned port);
    ~QueueSink();

protected:
    virtual Boolean continuePlaying();

protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);

    Writer *fWriter;
    unsigned fPort;
    Frame *frame;
    unsigned char *dummyBuffer;
    size_t seqNum;
};

#endif
