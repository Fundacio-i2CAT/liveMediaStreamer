/*
 *  H265QueueServerMediaSubsession.hh - It consumes NAL units from the frame queue on demand
 *  Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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

#include "H265QueueServerMediaSubsession.hh"

H265QueueServerMediaSubsession*
H265QueueServerMediaSubsession::createNew(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId,
                          Boolean reuseFirstSource) 
{
    return new H265QueueServerMediaSubsession(conn, env, replica, readerId, reuseFirstSource);
}

H265QueueServerMediaSubsession::H265QueueServerMediaSubsession(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId, Boolean reuseFirstSource)
: H264or5QueueServerMediaSubsession(env, replica, readerId, reuseFirstSource), fConn(conn)
{

}

H265QueueServerMediaSubsession::~H265QueueServerMediaSubsession() 
{

}

FramedSource* H265QueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    //TODO: WTF
    estBitrate = 2000; // kbps, estimate
    
    return H265VideoStreamDiscreteFramer::createNew(envir(), replicator->createStreamReplica());
}

RTPSink* H265QueueServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) 
{
    return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

RTCPInstance* H265QueueServerMediaSubsession::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                   unsigned char const* cname, RTPSink* sink)
{
    //TODO: reach setting id as the RTP port (as done for RTPConnection)
    size_t id = rand();

    ConnRTCPInstance* newRTCPInstance = ConnRTCPInstance::createNew(fConn, &envir(), RTCPgs, totSessionBW, sink);
    newRTCPInstance->setId(id);
    fConn->addConnectionRTCPInstance(id, newRTCPInstance);

    return newRTCPInstance;
}