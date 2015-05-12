/*
 *  H264QueueSource - A live 555 source which consumes frames from a LMS H264 queue
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
 
#ifndef _H264_OR_5_QUEUE_SOURCE_HH
#define _H264_OR_5_QUEUE_SOURCE_HH

#include <liveMedia.hh>
#include "QueueSource.hh"


class H264or5QueueSource: public QueueSource {

public:
    static H264or5QueueSource* createNew(UsageEnvironment& env, Reader *reader, int readerId);
    virtual void doGetNextFrame();

protected:
    H264or5QueueSource(UsageEnvironment& env, Reader *reader, int readerId);
};

#endif
