/*
 *  Connection.cpp - Class that represents a RTP transmission.
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 */

#include "Connection.hh"
#include "Utils.hh"
#include "UltraGridAudioRTPSink.hh"
#include "UltraGridVideoRTPSink.hh"
#include "H264VideoStreamSampler.hh"
#include "H264StartCodeInjector.hh"
#include <GroupsockHelper.hh>

Connection::Connection(UsageEnvironment* env, FramedSource *source) : 
                       fEnv(env), fSource(source), fSink(NULL)
{
}

Connection::~Connection()
{
    if (fSink) {
        fSink->stopPlaying();
    }
}

void Connection::afterPlaying(void* clientData) {
    MediaSink *clientSink = (MediaSink*) clientData;
    clientSink->stopPlaying();
}

void Connection::startPlaying()
{
    if (fSink){
        fSink->startPlaying(*fSource, &Connection::afterPlaying, fSink);
    }
}

void Connection::stopPlaying()
{
    if (fSink){
        fSink->stopPlaying();
    }
}

bool Connection::setup() 
{
    if (!specificSetup()) {
        return false;
    }

    startPlaying();
    return true;
}

////////////////////
// RTP CONNECTION //
////////////////////

RTPConnection::RTPConnection(UsageEnvironment* env, FramedSource* source,
                             std::string ip, unsigned port) :
                             Connection(env, source), fIp(ip), fPort(port)
{ 

}

RTPConnection::~RTPConnection()
{
    if (fSink) {
        Medium::close(rtcp);
    }

    if (rtpGroupsock) {
        delete rtpGroupsock;
    }

    if (rtcpGroupsock) {
        delete rtcpGroupsock;
    }
}

bool RTPConnection::specificSetup() 
{
    if (!firstStepSetup()) {
        return false;
    }

    if (!additionalSetup()) {
        return false;
    }

    if (!finalRTCPSetup()) {
        return false;
    }

    return true;
}

bool RTPConnection::firstStepSetup()
{
    unsigned serverPort = INITIAL_SERVER_PORT;
    destinationAddress.s_addr = our_inet_addr(fIp.c_str());
    
    if ((fPort%2) != 0) {
        utils::errorMsg("Port MUST be an even number");
        return false;
    }
    
    const Port rtpPort(fPort);
    const Port rtcpPort(fPort+1);
    
    for (;;serverPort+=2){
        rtpGroupsock = new Groupsock(*fEnv, destinationAddress, Port(serverPort), TTL);
        rtcpGroupsock = new Groupsock(*fEnv, destinationAddress, Port(serverPort+1), TTL);
        if (rtpGroupsock->socketNum() < 0 || rtcpGroupsock->socketNum() < 0) {
            delete rtpGroupsock;
            delete rtcpGroupsock;
            continue; // try again
        }
        break;
    }
    
    rtpGroupsock->changeDestinationParameters(destinationAddress, rtpPort, TTL);
    rtcpGroupsock->changeDestinationParameters(destinationAddress, rtcpPort, TTL);

    return true;
}

bool RTPConnection::finalRTCPSetup()
{
    RTPSink *rtpSink = NULL;

    if (!fSink) {
        utils::errorMsg("Error in finalRTCPSetup. Sink is NULL");
        return false;
    }

    rtpSink = dynamic_cast<RTPSink*>(fSink);

    if (!rtpSink) {
        utils::errorMsg("Error in finalRTCPSetup. Sink MUST be a RTPSink");
        return false;
    }

    const unsigned maxCNAMElen = 100;
    unsigned char CNAME[maxCNAMElen+1];
    gethostname((char*)CNAME, maxCNAMElen);
    CNAME[maxCNAMElen] = '\0';
    rtcp = RTCPInstance::createNew(*fEnv, rtcpGroupsock, 5000, CNAME,
                                   rtpSink, NULL, False);

    return true;
}

/////////////////////
// DASH CONNECTION //
/////////////////////

DASHConnection::DASHConnection(UsageEnvironment* env, FramedSource* source, std::string fileName,
                               std::string quality, bool reInit, uint32_t segmentTime, uint32_t initSegment) :
                               Connection(env, source), fFileName(fileName), fReInit(reInit),
                               fSegmentTime(segmentTime), fInitSegment(initSegment)
{

}

DASHConnection::~DASHConnection()
{
}

bool DASHConnection::specificSetup()
{
    return true;
} 


/////////////////////////
// RAW RTP CONNECTIONS //
/////////////////////////

VideoConnection::VideoConnection(UsageEnvironment* env, FramedSource* source,
                                 std::string ip, unsigned port, VCodecType codec) : 
                                 RTPConnection(env, source, ip, port), fCodec(codec)
{
    
}

bool VideoConnection::additionalSetup()
{
    switch(fCodec){
        case H264:
            fSink = H264VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            fSource = H264VideoStreamDiscreteFramer::createNew(*fEnv, fSource);
            break;
        case VP8:
            fSink = VP8VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            break;
        default:
            fSink = NULL;
            break;
    }

    if (!fSink) {
        return false;
    }

    return true;
}


AudioConnection::AudioConnection(UsageEnvironment* env, FramedSource *source,
                                 std::string ip, unsigned port, ACodecType codec,
                                 unsigned channels, unsigned sampleRate, SampleFmt sampleFormat) :
                                 RTPConnection(env, source, ip, port), fCodec(codec),
                                 fChannels(channels), fSampleRate(sampleRate),
                                 fSampleFormat(sampleFormat)
{
    
}

bool AudioConnection::additionalSetup()
{
    unsigned char payloadType = 97;
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
    
    if (fCodec == MP3){
        fSink =  MPEG1or2AudioRTPSink::createNew(*fEnv, rtpGroupsock);
    } else {
        fSink =  SimpleRTPSink::createNew(*fEnv, rtpGroupsock, payloadType,
                                         fSampleRate, "audio", 
                                         codecStr.c_str(),
                                         fChannels, False);
    }

    if (!fSink) {
        return false;
    }

    return true;
}

///////////////////////////////
// ULTRAGRID RTP CONNECTIONS //
///////////////////////////////

UltraGridVideoConnection::UltraGridVideoConnection(UsageEnvironment* env, FramedSource *source, 
                                                   std::string ip, unsigned port, VCodecType codec) :
                                                   RTPConnection(env, source, ip, port), fCodec(codec)
{
}

bool UltraGridVideoConnection::additionalSetup()
{

    switch(fCodec){
        case H264:
            fSource = H264VideoStreamSampler::createNew(*fEnv, fSource, true);
            fSink = UltraGridVideoRTPSink::createNew(*fEnv, rtpGroupsock);
            break;
        default:
            fSink = NULL;
            break;
    }

    if (!fSink) {
        utils::errorMsg("UltraGridVideoConnection could not be created");
        return false;
    }

    return true;
}

UltraGridAudioConnection::UltraGridAudioConnection(UsageEnvironment* env, FramedSource *source,
                                                   std::string ip, unsigned port, ACodecType codec,
                                                   unsigned channels, unsigned sampleRate,
                                                   SampleFmt sampleFormat) :
				RTPConnection(env, source, ip, port), fCodec(codec),
				fChannels(channels), fSampleRate(sampleRate),
				fSampleFormat(sampleFormat)
{

}

bool UltraGridAudioConnection::additionalSetup()
{

    switch(fCodec){
        case MP3:
        	fSink =  UltraGridAudioRTPSink::createNew(*fEnv, rtpGroupsock, fCodec,
        			                        fChannels, fSampleRate, fSampleFormat);
            break;
        default:
            fSink = NULL;
            break;
    }

	if (!fSink) {
		utils::errorMsg("UltraGridAudioConnection could not be created");
		return false;
	}

    return true;
}

////////////////////////
// MPEG-TS CONNECTION //
////////////////////////

MpegTsConnection::MpegTsConnection(UsageEnvironment* env, std::string ip, unsigned port) 
: RTPConnection(env, NULL, ip, port)
{
    tsFramer = MPEG2TransportStreamFromESSource::createNew(*env);
}

bool MpegTsConnection::addVideoSource(FramedSource* source, VCodecType codec)
{
    FramedSource* startCodeInjector;

    if (codec != H264) {
        utils::errorMsg("Error creating MPEG-TS Connection. Only H264 video codec is valid");
        return false;
    }

    if (!tsFramer) {
        utils::errorMsg("Error creating MPEG-TS Connection. MPEG2TransportStreamFromESSource is NULL");
        return false;
    }

    startCodeInjector = H264StartCodeInjector::createNew(*fEnv, source);
    tsFramer->addNewVideoSource(startCodeInjector, 5/*mpegVersion: H.264*/);

    return true;
}

bool MpegTsConnection::addAudioSource(FramedSource* source, ACodecType codec)
{
    //TODO:implement ADTS header injector in order to properly implement this method
    // FramedSource* adtsHeaderInjector;

    // if (codec != AAC) {
    //     utils::errorMsg("Error creating MPEG-TS Connection. Only AAC audio codec is valid");
    //     return false;
    // }

    // if (!tsFramer) {
    //     utils::errorMsg("Error creating MPEG-TS Connection. MPEG2TransportStreamFromESSource is NULL");
    //     return false;
    // }

    // adtsHeaderInjector = AacAdtsHeaderInjector::createNew(*fEnv, source);
    // tsFramer->addNewAudioSource(adtsHeaderInjector, 4/*mpegVersion: AAC*/);
    utils::errorMsg("Adding audio sources to MPEG-TS is not support for the moment");
    return false;
}
    
bool MpegTsConnection::additionalSetup()
{
    fSource = tsFramer;

    fSink = SimpleRTPSink::createNew(*fEnv, rtpGroupsock, 33, 90000, "video", 
                                     "MP2T", 1, True, False /*no 'M' bit*/);

    if (!fSink) {
        utils::errorMsg("Error setting up MpegTsConnection. Sink could not be created");
        return false;
    }

    return true;
}


//////////////////////
// DASH CONNECTIONS //
//////////////////////

DashVideoConnection::DashVideoConnection(UsageEnvironment* env, FramedSource *source, 
                                         std::string fileName, std::string quality, 
                                         bool reInit, uint32_t segmentTime, uint32_t initSegment, 
                                         VCodecType codec, uint32_t fps) : 
                                         DASHConnection(env, source, fileName, quality, 
                                            reInit, segmentTime, initSegment), 
                                         fCodec(codec), fFps(fps)
{

}

bool DashVideoConnection::additionalSetup()
{
    switch(fCodec){
        case H264:
            fSink = DashFileSink::createNew(*fEnv, fFileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4v", ONLY_VIDEO, false);
            fSource = DashSegmenterVideoSource::createNew(*fEnv, fSource, fReInit, fFps, fSegmentTime);
            break;
        default:
            fSink = NULL;
            break;
    }
    
    if (!fSink) {
        utils::errorMsg("DashVideoConnection could not be created");
        return false;
    }

    return true;
}


DashAudioConnection::DashAudioConnection(UsageEnvironment* env, FramedSource *source, 
                                         std::string fileName, std::string quality, 
                                         bool reInit, uint32_t segmentTime, uint32_t initSegment, 
                                         ACodecType codec, unsigned channels, 
                                         unsigned sampleRate, SampleFmt sampleFormat) : 
                                         DASHConnection(env, source, fileName, quality,
                                            reInit, segmentTime, initSegment), 
                                        fCodec(codec), fChannels(channels), 
                                        fSampleRate(sampleRate), fSampleFormat(sampleFormat)
{

}

bool DashAudioConnection::additionalSetup() 
{
    switch (fCodec) {
        case AAC:
            fSink = DashFileSink::createNew(*fEnv, fFileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4a", ONLY_AUDIO, false);
            fSource = DashSegmenterAudioSource::createNew(*fEnv, fSource, fReInit, fSegmentTime, fSampleRate);
            break;
        default:
            fSink = NULL;
            break;
    }

    if (!fSink) {
        utils::errorMsg("DashAudioConnection could not be created");
        return false;
    }

    return true;   
}
