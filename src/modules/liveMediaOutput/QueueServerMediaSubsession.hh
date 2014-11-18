/*
 *  QueueServerMediaSubsession.cpp - A generic subsession class for our frame queue
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


#ifndef _QUEUE_SERVER_MEDIA_SUBSESSION_HH
#define _QUEUE_SERVER_MEDIA_SUBSESSION_HH

#include <liveMedia/liveMedia.hh>
#include "../../IOInterface.hh"


class QueueServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
    int getReaderId() {return fReaderId;};
    virtual ~QueueServerMediaSubsession();
    
protected: 
    QueueServerMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator,
                               int readerId, Boolean reuseFirstSource);

    virtual void getStreamParameters(unsigned clientSessionId,
            netAddressBits clientAddress,
            Port const& clientRTPPort,
            Port const& clientRTCPPort,
            int tcpSocketNum,
            unsigned char rtpChannelId,
            unsigned char rtcpChannelId,
            netAddressBits& destinationAddress,
            uint8_t& destinationTTL,
            Boolean& isMulticast,
            Port& serverRTPPort,
            Port& serverRTCPPort,
            void*& streamToken);

protected:
    StreamReplicator* fReplicator;
    int fReaderId;
};

#endif
