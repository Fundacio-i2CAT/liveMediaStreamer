/*
 *  VP8QueueServerMediaSubsession.hh - It consumes VP8 frames from the frame queue on demand
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

#ifndef _VP8_SERVER_MEDIA_SUBSESSION_HH
#define _VP8_SERVER_MEDIA_SUBSESSION_HH

#include <liveMedia.hh>
#include "QueueServerMediaSubsession.hh"
#include "Connection.hh"

class VP8QueueServerMediaSubsession: public QueueServerMediaSubsession {
public:
    static VP8QueueServerMediaSubsession*
    createNew(Connection* conn, UsageEnvironment& env, StreamReplicator* replica, int readerId, Boolean reuseFirstSource);
    
    std::vector<int> getReaderIds();

protected:
    VP8QueueServerMediaSubsession(Connection* conn, UsageEnvironment& env,
                                  StreamReplicator* replica, int readerId, Boolean reuseFirstSource);
  
    ~VP8QueueServerMediaSubsession();

protected: // redefined virtual functions
    FramedSource* createNewStreamSource(unsigned clientSessionId,
                                        unsigned& estBitrate);
    RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                              unsigned char rtpPayloadTypeIfDynamic,
                              FramedSource* inputSource);
    RTCPInstance* createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                   unsigned char const* cname, RTPSink* sink);

private:
    StreamReplicator* replicator;
    int reader;
    Connection* fConn;
};

#endif

