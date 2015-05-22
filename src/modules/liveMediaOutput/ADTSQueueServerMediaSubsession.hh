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

#ifndef _ADTS_QUEUE_SERVER_MEDIA_SUBSESSION_HH
#define _ADTS_QUEUE_SERVER_MEDIA_SUBSESSION_HH

#include "QueueServerMediaSubsession.hh"

class ADTSQueueServerMediaSubsession: public QueueServerMediaSubsession {
    
public:
    static ADTSQueueServerMediaSubsession*
        createNew(UsageEnvironment& env, StreamReplicator* replica, int readerId, 
                  unsigned channels, unsigned sampleRate, Boolean reuseFirstSource);

    void checkForAuxSDPLine1();
    void afterPlayingDummy1();
    std::vector<int> getReaderIds();

protected:
    ADTSQueueServerMediaSubsession(UsageEnvironment& env, StreamReplicator* replica, int readerId, 
                                   unsigned channels, unsigned sampleRate, Boolean reuseFirstSource);

    virtual ~ADTSQueueServerMediaSubsession();
    void setDoneFlag() { fDoneFlag = ~0; }
    char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
    FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
    RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);

private:
    StreamReplicator* replicator;
    int reader;
    unsigned fChannels;
    unsigned fSampleRate;
    char* fAuxSDPLine;
    char fDoneFlag; 
    RTPSink* fDummyRTPSink;
};

#endif