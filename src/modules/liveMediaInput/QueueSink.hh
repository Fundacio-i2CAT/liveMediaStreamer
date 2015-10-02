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

#define DUMMY_RECEIVE_BUFFER_SIZE 200000

class QueueSink: public MediaSink {

public:
    
    /**
    * Constructor wrapper
    * @param env Live555 environement
    * @param port network port of the incoming stream
    * @param filter pointer of this source, if defined it contains additional headers
    * @return Pointer to the object if succeded and NULL if not
    */
    static QueueSink* createNew(UsageEnvironment& env, unsigned port, FramedFilter* filter);
    
    /**
    * @return port number of the incoming stream
    */
    unsigned getPort() {return fPort;};
    
    /**
     * Sets the frame pointer where the next incoming framed will be placed.
     * @param frame pointer where the incoming frame will be copied
     * @return true if successfully updated frame, false if not ready to set next frame.
     */
    bool setFrame(Frame *f);
    
    /**
    * @return filter pointer of the source.
    */
    FramedFilter* getFilter() {return fFilter;};

protected:
    QueueSink(UsageEnvironment& env, unsigned port, FramedFilter* filter);
    ~QueueSink();
    
    void static staticContinuePlaying(QueueSink *sink);
    virtual Boolean continuePlaying();
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);

protected:
    unsigned fPort;
    Frame *frame;
    
    unsigned char *dummyBuffer;
    bool nextFrame;
    FramedFilter* fFilter;
};

#endif
