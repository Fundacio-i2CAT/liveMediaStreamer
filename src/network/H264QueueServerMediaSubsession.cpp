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

#ifndef _QUEUE_SOURCE_HH
#include "H264QueueSource.hh"
#endif

#include "H264QueueServerMediaSubsession.hh"

H264QueueServerMediaSubsession*
H264QueueServerMediaSubsession::createNew(UsageEnvironment& env,
                          QueueSource *source,
                          Boolean reuseFirstSource) {
  return new H264QueueServerMediaSubsession(env, source, reuseFirstSource);
}

H264QueueServerMediaSubsession::H264QueueServerMediaSubsession(UsageEnvironment& env,
                                    QueueSource *source, Boolean reuseFirstSource)
  : QueueServerMediaSubsession(env, source, reuseFirstSource),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL) {
}

H264QueueServerMediaSubsession::~H264QueueServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H264QueueServerMediaSubsession* subsess = (H264QueueServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H264QueueServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H264QueueServerMediaSubsession* subsess = (H264QueueServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264QueueServerMediaSubsession::checkForAuxSDPLine1() {
  char const* dasl;

  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
                  (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H264QueueServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}

FramedSource* H264QueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    //TODO: WTF
    estBitrate = 500; // kbps, estimate
    
    if (fSource != NULL && dynamic_cast<H264QueueSource*>(fSource)){
        // Create a framer for the Video Elementary Stream:
        return H264VideoStreamDiscreteFramer::createNew(envir(), fSource);
    }
    
    return NULL;
}

RTPSink* H264QueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
