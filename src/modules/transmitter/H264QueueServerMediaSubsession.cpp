/*
 *  H264QueueServerMediaSubsession.hh - It consumes NAL units from the frame queue on demand
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
 *            Marc Palau <marc.palau@i2cat.net>
 *            
 */

#include "H264QueueServerMediaSubsession.hh"
#include "../../Utils.hh"
#include "H264or5QueueSource.hh"

H264QueueServerMediaSubsession*
H264QueueServerMediaSubsession::createNew(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId,
                          Boolean reuseFirstSource)
{
    return new H264QueueServerMediaSubsession(conn, env, replica, readerId, reuseFirstSource);
}

H264QueueServerMediaSubsession::H264QueueServerMediaSubsession(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId, Boolean reuseFirstSource)
: H264or5QueueServerMediaSubsession(env, replica, readerId, reuseFirstSource), fConn(conn)
{
}

H264QueueServerMediaSubsession::~H264QueueServerMediaSubsession() 
{
}

FramedSource* H264QueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
    //TODO: WTF
    estBitrate = 2000; // kbps, estimate
    
    return H264VideoStreamDiscreteFramer::createNew(envir(), replicator->createStreamReplica());
}

RTPSink* H264QueueServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) 
{
    H264or5QueueSource *source;
    
    if ((source = dynamic_cast<H264or5QueueSource *>(replicator->inputSource()))){
        if (source->parseExtradata()){
            return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 
                                           source->getSPS(), source->getSPSSize(),
                                           source->getPPS(), source->getPPSSize());
        }
    }
    
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

RTCPInstance* H264QueueServerMediaSubsession::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                   unsigned char const* cname, RTPSink* sink)
{
    //TODO: reach setting id as the RTP port (as done for RTPConnection)
    size_t id = rand();

    ConnRTCPInstance* newRTCPInstance = ConnRTCPInstance::createNew(fConn, &envir(), RTCPgs, totSessionBW, sink);
    newRTCPInstance->setId(id);
    fConn->addConnectionRTCPInstance(id, newRTCPInstance);

    return newRTCPInstance;
}