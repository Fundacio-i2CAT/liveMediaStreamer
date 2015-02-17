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
 *            
 */

#include "H264QueueSource.hh"
#include "H264QueueServerMediaSubsession.hh"
#include <string>

H264QueueServerMediaSubsession*
H264QueueServerMediaSubsession::createNew(UsageEnvironment& env,
                          StreamReplicator* replica, int readerId,
                          Boolean reuseFirstSource) 
{
    return new H264QueueServerMediaSubsession(env, replica, readerId, reuseFirstSource);
}

H264QueueServerMediaSubsession::H264QueueServerMediaSubsession(UsageEnvironment& env,
                          StreamReplicator* replica, int readerId, Boolean reuseFirstSource)
: QueueServerMediaSubsession(env, reuseFirstSource), replicator(replica), reader(readerId),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL) 
{
}

H264QueueServerMediaSubsession::~H264QueueServerMediaSubsession() 
{
  //NOTE: is this needed?
  Medium::close(replicator);
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) 
{
  H264QueueServerMediaSubsession* subsess = (H264QueueServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H264QueueServerMediaSubsession::afterPlayingDummy1() {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H264QueueServerMediaSubsession* subsess = (H264QueueServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264QueueServerMediaSubsession::checkForAuxSDPLine1() {
  char const* dasl;

  if (fAuxSDPLine != NULL) {
    
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    
    setDoneFlag();
  } else {
    
    int uSecsToDelay = 100000; 
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
                  (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H264QueueServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
    if (fAuxSDPLine != NULL) return fAuxSDPLine; 

    if (fDummyRTPSink == NULL) {
        fDummyRTPSink = rtpSink;
        fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&fDoneFlag);
    return fAuxSDPLine;
}

FramedSource* H264QueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    //TODO: WTF
    estBitrate = 2000; // kbps, estimate
    
    return H264VideoStreamDiscreteFramer::createNew(envir(), replicator->createStreamReplica());
}

RTPSink* H264QueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

std::vector<int> H264QueueServerMediaSubsession::getReaderIds()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}
