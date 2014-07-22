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

#include "VP8QueueServerMediaSubsession.hh"
#include "QueueSource.hh"


VP8QueueServerMediaSubsession*
VP8QueueServerMediaSubsession::createNew(UsageEnvironment& env,
                          StreamReplicator* replicator, int readerId,
                          Boolean reuseFirstSource) {
    return new VP8QueueServerMediaSubsession(env, replicator, readerId, reuseFirstSource);
}

VP8QueueServerMediaSubsession::VP8QueueServerMediaSubsession(UsageEnvironment& env,
                          StreamReplicator* replicator, int readerId, 
                          Boolean reuseFirstSource)
  : QueueServerMediaSubsession(env, replicator, readerId, reuseFirstSource) {
}

FramedSource* VP8QueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    //TODO: WTF
    estBitrate = 2000; // kbps, estimate

    return fReplicator->createStreamReplica();
}

RTPSink* VP8QueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
    return VP8VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
