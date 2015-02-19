/*
 *  MPEGTSQueueServerMediaSubsession.hh - A subsession class for MPEGTS
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


#ifndef _MPEGTS_QUEUE_SERVER_MEDIA_SUBSESSION_HH
#define _MPEGTS_QUEUE_SERVER_MEDIA_SUBSESSION_HH

#include <liveMedia.hh>
#include "../../IOInterface.hh"
#include "QueueServerMediaSubsession.hh"


class MPEGTSQueueServerMediaSubsession: public QueueServerMediaSubsession {
public:
    static MPEGTSQueueServerMediaSubsession*
        createNew(UsageEnvironment& env, Boolean reuseFirstSource);
    
    /**
    * Adds a video source to an MPEGTS subsession
    * @param codec represents the video codec of the source to add
    * @param replicator it is the replicator from where the source is created
    * @param readerId it is the id of the reader associated with this replicator
    * @return True if succeded and false if not
    */
    bool addVideoSource(VCodecType codec, StreamReplicator* replicator, int readerId);
    
    /**
    * Adds an audio source to an MPEGTS subsession
    * @param codec represents the audio codec of the source to add
    * @param replicator it is the replicator from where the source is created
    * @param readerId it is the id of the reader associated with this replicator
    * @return True if succeded and false if not
    */
    bool addAudioSource(ACodecType codec, StreamReplicator* replicator, int readerId);
    
    /**
    * Get the associated readers of this connection
    * @return a vector of readers ids without any particular order
    */
    std::vector<int> getReaderIds();

protected:
  MPEGTSQueueServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource);

  ~MPEGTSQueueServerMediaSubsession();

protected: 
  FramedSource* createNewStreamSource(unsigned clientSessionId,
                          unsigned& estBitrate);
  RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                    FramedSource* inputSource);

private:
    StreamReplicator* aReplicator;
    StreamReplicator* vReplicator;
    
    int aReaderId;
    int vReaderId;
};

#endif