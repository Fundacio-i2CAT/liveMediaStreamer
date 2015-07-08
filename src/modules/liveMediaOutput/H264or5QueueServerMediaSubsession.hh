/*
 *  H264or5QueueServerMediaSubsession.hh - It consumes NAL units from the frame queue on demand
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
 *  Authors: Marc Palau <marc.palau@i2cat.net>
 *            
 */

#ifndef _H264_OR_5_SERVER_MEDIA_SUBSESSION_HH
#define _H264_OR_5_SERVER_MEDIA_SUBSESSION_HH

#include <liveMedia.hh>
#include "QueueServerMediaSubsession.hh"
#include "../../Utils.hh"

class H264or5QueueServerMediaSubsession: public QueueServerMediaSubsession {

public:
    std::vector<int> getReaderIds();

protected:
    H264or5QueueServerMediaSubsession(UsageEnvironment& env, StreamReplicator* replica, 
                                 int readerId, Boolean reuseFirstSource);

    virtual ~H264or5QueueServerMediaSubsession();

    StreamReplicator* replicator;

private:
    int reader;
};

#endif