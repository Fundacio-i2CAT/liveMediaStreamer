/*
 *  AudioQueueServerMediaSubsession.hh - It consumes audio frames from the frame queue on demand
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

#include "AudioQueueServerMediaSubsession.hh"
#include "QueueSource.hh"
#include "../../AVFramedQueue.hh"


AudioQueueServerMediaSubsession*
AudioQueueServerMediaSubsession::createNew(UsageEnvironment& env,
                          Reader *reader, int readerId,
                          Boolean reuseFirstSource) {
  return new AudioQueueServerMediaSubsession(env, reader, readerId, reuseFirstSource);
}

AudioQueueServerMediaSubsession::AudioQueueServerMediaSubsession(UsageEnvironment& env,
                                    Reader *reader, int readerId, Boolean reuseFirstSource)
  : QueueServerMediaSubsession(env, reader, readerId, reuseFirstSource) {
}

FramedSource* AudioQueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    AudioFrameQueue *aQueue;
    QueueSource *qSource;
    unsigned bitsPerSecond = 0;
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(fReader->getQueue())) == NULL){
        return NULL;
    }
    
    if (aQueue->getSampleFmt() == S16){
        bitsPerSecond = aQueue->getSampleRate()*2*aQueue->getChannels();
    } else if (aQueue->getSampleFmt() == U8){
        bitsPerSecond = aQueue->getSampleRate()*1*aQueue->getChannels();
    }
    
    qSource = QueueSource::createNew(envir(), fReader, fReaderId);
    
    if (bitsPerSecond > 0){
        estBitrate = (bitsPerSecond+500)/1000; // kbps
    } else {
        estBitrate = 128; // kbps, estimate
    }
    return qSource;
}

RTPSink* AudioQueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
    AudioFrameQueue *aQueue;
    unsigned char payloadType = rtpPayloadTypeIfDynamic;
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(fReader->getQueue())) == NULL){
        return NULL;
    }
    
    if (aQueue->getCodec() == PCM && 
        aQueue->getSampleRate() == 44100 &&
        aQueue->getSampleFmt() == S16){
        if (aQueue->getChannels() == 2){
            payloadType = 10;
        } else if (aQueue->getChannels() == 1) {
            payloadType = 11;
        }
    } else if ((aQueue->getCodec() == PCMU || aQueue->getCodec() == G711) && 
              aQueue->getSampleRate() == 8000 &&
              aQueue->getChannels() == 1){
        payloadType = 0;
    }
    
    if (aQueue->getCodec() == MP3){
        return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
    }
    
    return SimpleRTPSink
    ::createNew(envir(), rtpGroupsock, payloadType,
                aQueue->getSampleRate(), "audio", 
                utils::getAudioCodecAsString(aQueue->getCodec()).c_str(),
                aQueue->getChannels(), False);
}
