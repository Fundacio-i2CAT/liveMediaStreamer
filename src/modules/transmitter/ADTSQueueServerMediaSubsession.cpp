/*
 *  ADTSQueueServerMediaSubsession.hh - It consumes AAC frame with ADTS header from the frame queue on demand
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

#include "ADTSQueueServerMediaSubsession.hh"
#include "CustomMPEG4GenericRTPSink.hh"
#include "ADTSStreamParser.hh"

ADTSQueueServerMediaSubsession*
ADTSQueueServerMediaSubsession::createNew(Connection* conn, UsageEnvironment& env, StreamReplicator* replica, int readerId, 
                                          unsigned channels, unsigned sampleRate, Boolean reuseFirstSource)
{
    return new ADTSQueueServerMediaSubsession(conn, env, replica, readerId, channels, sampleRate, reuseFirstSource);
}

ADTSQueueServerMediaSubsession
::ADTSQueueServerMediaSubsession(Connection* conn, UsageEnvironment& env, StreamReplicator* replica, int readerId, 
                                 unsigned channels, unsigned sampleRate, Boolean reuseFirstSource) :
QueueServerMediaSubsession(env, reuseFirstSource), replicator(replica), reader(readerId), fChannels(channels), 
fSampleRate(sampleRate), fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fConn(conn)
{

}

ADTSQueueServerMediaSubsession::~ADTSQueueServerMediaSubsession() 
{
    delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) 
{
    ADTSQueueServerMediaSubsession* subsess = (ADTSQueueServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void ADTSQueueServerMediaSubsession::afterPlayingDummy1() 
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) 
{
    ADTSQueueServerMediaSubsession* subsess = (ADTSQueueServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void ADTSQueueServerMediaSubsession::checkForAuxSDPLine1() 
{
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

char const* ADTSQueueServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) 
{
    if (fAuxSDPLine != NULL) return fAuxSDPLine; 

    if (fDummyRTPSink == NULL) {
        fDummyRTPSink = rtpSink;
        fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&fDoneFlag);
    return fAuxSDPLine;
}

FramedSource* ADTSQueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
    estBitrate = 128; // kbps, estimate
    return ADTSStreamParser::createNew(envir(), replicator->createStreamReplica());
}

RTPSink* ADTSQueueServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) 
{
      return CustomMPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 
                                                  fSampleRate, "audio", "AAC-hbr", fChannels);
}

RTCPInstance* ADTSQueueServerMediaSubsession::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                   unsigned char const* cname, RTPSink* sink)
{
    //TODO: reach setting id as the RTP port (as done for RTPConnection)
    size_t id = rand();

    ConnRTCPInstance* newRTCPInstance = ConnRTCPInstance::createNew(fConn, &envir(), RTCPgs, totSessionBW, sink);
    newRTCPInstance->setId(id);
    fConn->addConnectionRTCPInstance(id, newRTCPInstance);

    return newRTCPInstance;
}

std::vector<int> ADTSQueueServerMediaSubsession::getReaderIds()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

