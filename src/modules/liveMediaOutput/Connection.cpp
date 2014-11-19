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

#include <GroupsockHelper.hh>
#include "Connection.hh"
#include "Utils.hh"
#include "UltraGridVideoRTPSink.hh"

Connection::Connection(UsageEnvironment* env, FramedSource *source) : 
                       fEnv(env), fSource(source), fSink(NULL)
{
}

Connection::~Connection()
{
    Medium::close(fSource);

    if (sink) {
        sink->stopPlaying();
        Medium::close(sink);
    }
}

void Connection::afterPlaying(void* clientData) {
    MediaSink *clientSink = (MediaSInk*) clientData;
    clientSink->stopPlaying();
}

void Connection::startPlaying()
{
    if (sink){
        sink->startPlaying(*fSource, &Connection::afterPlaying, sink);
    }
}

void Connection::stopPlaying()
{
    if (sink){
        sink->stopPlaying();
    }
}

bool Connection::setup() 
{
    if (!specificSetup()) {
        return false;
    }

    startPlaying()
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
    if (sink) {
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
    
    if ((port%2) != 0){
        utils::errorMsg("Port MUST be an even number");
        return false;
    }
    
    const Port rtpPort(port);
    const Port rtcpPort(port+1);
    
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

    if (!sink) {
        utils::errorMsg("Error in finalRTCPSetup. Sink is NULL");
        return false;
    }

    rtpSink = dynamic_cast<RTPSink*>(sink);

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

DASHConnection::DASHConnection(UsageEnvironment* env, FramedSource* source, std::string fileName) :
                               Connection(env, source), fFileName(fileName)
{


}

DASHConnection::~DASHConnection()
{
}

bool DASHConnection::specificSetup()
{

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
            sink = H264VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            fSource = H264VideoStreamDiscreteFramer::createNew(*fEnv, fSource);
            break;
        case VP8:
            sink = VP8VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            break;
        default:
            sink = NULL;
            break;
    }

    if (!sink) {
        return false;
    }

    return true;
}


AudioConnection::AudioConnection(UsageEnvironment* env, FramedSource *source
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
        sink =  MPEG1or2AudioRTPSink::createNew(*fEnv, rtpGroupsock);
    } else {
        sink =  SimpleRTPSink::createNew(*fEnv, rtpGroupsock, payloadType,
                                         fSampleRate, "audio", 
                                         codecStr.c_str(),
                                         fChannels, False);
    }

    if (!sink) {
        return false;
    }

    return true;
}

///////////////////////////////
// ULTRAGRID RTP CONNECTIONS //
///////////////////////////////

UltraGridVideoConnection* UltraGridVideoConnection::createNew(UsageEnvironment* env, 
                                                              std::string ip, unsigned port, 
                                                              FramedSource *source) 
{
    UltraGridVideoConnection* conn = new UltraGridVideoConnection(env, ip, port, source);

    if (!conn->setup()) {
        delete conn;
        return NULL;
    }

    return conn;
}

UltraGridVideoConnection::UltraGridVideoConnection(UsageEnvironment* env, 
                                                   std::string ip, unsigned port, 
                                                   FramedSource *source) : 
                                                   Connection(env, ip, port, source)
{

}

bool UltraGridVideoConnection::setup()
{
    sink = UltraGridVideoRTPSink::createNew(*fEnv, rtpGroupsock);
    fSource = H264VideoStreamDiscreteFramer::createNew(*fEnv, fSource);

    if (!sink) {
        utils::errorMsg("UltraGridVideoConnection could not be created");
        return false;
    }

    const unsigned maxCNAMElen = 100;
    unsigned char CNAME[maxCNAMElen+1];
    gethostname((char*)CNAME, maxCNAMElen);
    CNAME[maxCNAMElen] = '\0';
    rtcp = RTCPInstance::createNew(*fEnv, rtcpGroupsock, 5000, CNAME,
                                   sink, NULL, False);

    startPlaying();
    return true;
}



UltraGridAudioConnection* UltraGridAudioConnection::createNew(UsageEnvironment* env, 
                                                              std::string ip, unsigned port, 
                                                              FramedSource *source) 
{
    UltraGridAudioConnection* conn = new UltraGridAudioConnection(env, ip, port, source);
    return conn;
}

UltraGridAudioConnection::UltraGridAudioConnection(UsageEnvironment* env, 
                                                   std::string ip, unsigned port, 
                                                   FramedSource *source) : 
                                                   Connection(env, ip, port, source)
{

}

bool UltraGridAudioConnection::setup()
{
    //TODO: implement
    return true;
}


//////////////////////
// DASH CONNECTIONS //
//////////////////////

DashVideoConnection::DashVideoConnection(UsageEnvironment* env, std::string fileName, 
                                         FramedSource *source, VCodecType codec, 
                                         std::string quality, uint32_t fps, bool reInit, 
                                         uint32_t segmentTime, uint32_t initSegment) : 
                                         Connection(env, fileName, source), fCodec(codec), 
                                         fReInit(reInit), fFps(fps), fSegmentTime(segmentTime), 
                                         fInitSegment(initSegment)
{
    switch(fCodec){
        case H264:
            outputVideoFile = DashFileSink::createNew(*env, fileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4v", ONLY_VIDEO, false);
            fSource = DashSegmenterVideoSource::createNew(*fEnv, source, fReInit, fFps, fSegmentTime);
            break;
        default:
            sink = NULL;
            break;
    }
   if (outputVideoFile != NULL){
        //TODO??
    } else {
        utils::errorMsg("VideoConnection could not be created");
    }
    startPlaying();
}

DashAudioConnection::DashAudioConnection(UsageEnvironment* env, 
                                 std::string fileName, 
                                 FramedSource *source, ACodecType codec, 
                                 unsigned channels, unsigned sampleRate, 
                                 SampleFmt sampleFormat, std::string quality, bool reInit, uint32_t segmentTime, uint32_t initSegment) : Connection(env, fileName, source), 
                                 fCodec(codec), fChannels(channels), fSampleRate(sampleRate), 
                                 fSampleFormat(sampleFormat), fReInit(reInit), fSegmentTime(segmentTime), fInitSegment(initSegment)
{
    switch (fCodec) {
        case MPEG4_GENERIC:
            outputVideoFile = DashFileSink::createNew(*env, fileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4a", ONLY_AUDIO, false);
            fSource = DashSegmenterAudioSource::createNew(*fEnv, source, fReInit, fSegmentTime, fSampleRate);
            break;
        default:
            sink = NULL;
            break;
    }
   if (outputVideoFile != NULL){
        //TODO??
    } else {
        utils::errorMsg("AudioConnection could not be created");
    }
    startPlaying();    
}