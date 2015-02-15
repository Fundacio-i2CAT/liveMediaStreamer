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

Connection::Connection(UsageEnvironment* env) : 
                       fEnv(env)
{
}

Connection::~Connection()
{
}

void Connection::afterPlaying(void* clientData) {
    MediaSink *clientSink = (MediaSink*) clientData;
    clientSink->stopPlaying();
}

bool Connection::setup() 
{
    if (!specificSetup()) {
        return false;
    }

    if (!startPlaying()) {
        return false;
    }

    return true;
}

////////////////////
// RTSP CONNECTION //
////////////////////

RTSPConnection::RTSPConnection(UsageEnvironment* env, TxFormat txformat, RTSPServer *server,
                               std::string name_, std::string info, std::string desc) :
                               Connection(env), session(NULL), rtspServer(server), 
                               name(name_), tsFramer(NULL), format(txformat)
                   
{
    session = ServerMediaSession::createNew(*env, name.c_str(), info.c_str(), desc.c_str());
    if (session == NULL){
        utils::errorMsg("Failed creating new ServerMediaSession");
    }
    
    if (format == MPEGTS){
        tsFramer = MPEG2TransportStreamFromESSource::createNew(*env);
    }
}

bool RTSPConnection::addVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId)
{
    MediaSubsession *subsession = NULL;
    switch(codec){
        case H264:
            subsession = H264QueueServerMediaSubsession::createNew(env, replicators[readerId], readerId, False);
            break;
        case VP8:
            subsession =  VP8QueueServerMediaSubsession::createNew(env, replicators[readerId], readerId, False);
            break;
        default:
            break;
    }
    
    if (!subsession){
        return false;
    }
    
    session->addSubsession(subsession);
    
    return true;
}

bool RTSPConnection::addAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                                        unsigned channels, unsigned sampleRate, 
                                        SampleFmt sampleFormat, int readerId)
{
    MediaSubsession *subsession = NULL;
    switch(codec){
        //case MPEG4_GENERIC:
            //TODO
                        //printf("TODO createAudioMediaSubsession\n");
            //break;
        default:
            subsession = AudioQueueServerMediaSubsession::createNew(*(envir()), replicator,
                                                              readerId, codec, channels,
                                                              sampleRate, sampleFormat, False);
            break;
    }
    
    if (!subsession){
        return false;
    }
    
    session->addSubsession(subsession);
    
    return true;
}

bool RTSPConnection::startPlaying()
{
    if (rtspServer == NULL){
        return false;
    }
    
    if (session == NULL){
        return false;
    }

    rtspServer->addServerMediaSession(session);
    char* url = rtspServer->rtspURL(session);

    utils::infoMsg("Play " + id + " stream using the URL " + url);
    delete[] url;

    return true;
}

////////////////////
// RTP CONNECTION //
////////////////////

RTPConnection::RTPConnection(UsageEnvironment* env, FramedSource* source,
                             std::string ip, unsigned port) :
                             Connection(env), fIp(ip), fPort(port), 
                             fSource(source), fSink(NULL)
{ 

}

RTPConnection::~RTPConnection()
{   
    if (fSink) {
        fSink->stopPlaying();
        Medium::close(rtcp);
    }

    if (rtpGroupsock) {
        delete rtpGroupsock;
    }

    if (rtcpGroupsock) {
        delete rtcpGroupsock;
    }
}

bool RTPConnection::startPlaying()
{
    if (!fSink || !fSource) {
        utils::errorMsg("Cannot start playing, sink and/or source does not exist.");
        return false;
    }

    fSink->startPlaying(*fSource, &Connection::afterPlaying, fSink);
    return true;
}

void RTPConnection::stopPlaying()
{
    if (fSink){
        fSink->stopPlaying();
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
    // } else if (fCodec == AAC) {
    //     //NOTE: check ADTS File source to check how to construct the configuration string
    //     fSink = MPEG4GenericRTPSink::createNew(*fEnv, rtpGroupsock, payloadType, 
    //                                            fSampleRate, "audio", "AAC-hbr", 
    //                                            adtsSource->configStr(), fChannels);
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

    if (!source) {
        utils::errorMsg("Error adding video source to MPEG-TS Connection. Provided source is NULL");
        return false;
    }

    startCodeInjector = H264StartCodeInjector::createNew(*fEnv, source);
    tsFramer->addNewVideoSource(startCodeInjector, 5/*mpegVersion: H.264*/);

    return true;
}

bool MpegTsConnection::addAudioSource(FramedSource* source, ACodecType codec)
{
    if (codec != AAC) {
        utils::errorMsg("Error creating MPEG-TS Connection. Only AAC audio codec is valid");
        return false;
    }

    if (!tsFramer) {
        utils::errorMsg("Error creating MPEG-TS Connection. MPEG2TransportStreamFromESSource is NULL");
        return false;
    }

    if (!source) {
        utils::errorMsg("Error adding audio source to MPEG-TS Connection. Provided source is NULL");
        return false;
    }

    tsFramer->addNewAudioSource(source, 4/*mpegVersion: AAC*/);
    return true;
}
    
bool MpegTsConnection::additionalSetup()
{
    if (!tsFramer) {
        utils::errorMsg("Error creating MPEG-TS Connection. MPEG2TransportStreamFromESSource is NULL");
        return false;
    }
    
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
//    switch(fCodec){
//        case H264:
//            fSink = DashFileSink::createNew(*fEnv, fFileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4v", ONLY_VIDEO, false);
//            fSource = DashSegmenterVideoSource::createNew(*fEnv, fSource, fReInit, fFps, fSegmentTime);
//            break;
//        default:
//            fSink = NULL;
//            break;
//    }
//
//    if (!fSink) {
//        utils::errorMsg("DashVideoConnection could not be created");
//        return false;
//    }
//
//    return true;
	return false;
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
//    switch (fCodec) {
//        case AAC:
//            fSink = DashFileSink::createNew(*fEnv, fFileName.c_str(), MAX_DAT, True, quality.c_str(), fInitSegment, "m4a", ONLY_AUDIO, false);
//            fSource = DashSegmenterAudioSource::createNew(*fEnv, fSource, fReInit, fSegmentTime, fSampleRate);
//            break;
//        default:
//            fSink = NULL;
//            break;
//    }
//
//    if (!fSink) {
//        utils::errorMsg("DashAudioConnection could not be created");
//        return false;
//    }
//
//    return true;
	return false;
}
