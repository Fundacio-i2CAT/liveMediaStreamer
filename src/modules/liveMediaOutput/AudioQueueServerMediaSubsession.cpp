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
                          StreamReplicator* replicator, int readerId,
                          ACodecType codec, unsigned channels,
                          unsigned sampleRate, SampleFmt sampleFormat,
                          Boolean reuseFirstSource) {
  return new AudioQueueServerMediaSubsession(env, replicator, readerId, 
                                             codec, channels, sampleRate,
                                             sampleFormat, reuseFirstSource);
}

AudioQueueServerMediaSubsession::AudioQueueServerMediaSubsession(UsageEnvironment& env,
                          StreamReplicator* replicator, int readerId, 
                          ACodecType codec, unsigned channels,
                          unsigned sampleRate, SampleFmt sampleFormat,
                          Boolean reuseFirstSource)
  : QueueServerMediaSubsession(env, replicator, readerId, reuseFirstSource), fCodec(codec),
                          fChannels(channels), fSampleRate(sampleRate), fSampleFormat(sampleFormat) {
}

FramedSource* AudioQueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    
    unsigned bitsPerSecond = 0;
    
    if (fSampleFormat == S16){
        bitsPerSecond = fSampleRate*2*fChannels;
    } else if (fSampleFormat == U8){
        bitsPerSecond = fSampleRate*1*fChannels;
    }
    
    if (bitsPerSecond > 0){
        estBitrate = (bitsPerSecond+500)/1000; // kbps
    } else {
        estBitrate = 128; // kbps, estimate
    }
    return fReplicator->createStreamReplica();
}

RTPSink* AudioQueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
    unsigned char payloadType = rtpPayloadTypeIfDynamic;
    
    std::string codecStr = utils::getAudioCodecAsString(fCodec);
    
    if (fCodec == PCM && 
        fSampleFormat == S16) {
        codecStr = "L16";
        if (fSampleRate == 44100) {
            if (fChannels == 2){
                payloadType = 10;
            } else if (fChannels == 1) {
                payloadType = 11;
            }
        }
    } else if ((fCodec == PCMU || fCodec == G711) && 
                fSampleRate == 8000 &&
                fChannels == 1){
        payloadType = 0;
    }
    
    return SimpleRTPSink
    ::createNew(envir(), rtpGroupsock, payloadType,
                fSampleRate, "audio", 
                codecStr.c_str(),
                fChannels, False);
}
