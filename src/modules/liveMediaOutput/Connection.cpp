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
#include "H264or5StartCodeInjector.hh"
#include "H264QueueServerMediaSubsession.hh"
#include "H265QueueServerMediaSubsession.hh"
#include "VP8QueueServerMediaSubsession.hh"
#include "AudioQueueServerMediaSubsession.hh"
#include "ADTSQueueServerMediaSubsession.hh"
#include "ADTSStreamParser.hh"
#include "CustomMPEG4GenericRTPSink.hh"
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
                               name(name_), subsession(NULL), format(txformat), 
                               addedSub(false), started(false)
                   
{
    session = ServerMediaSession::createNew(*env, name.c_str(), info.c_str(), desc.c_str());
    if (session == NULL){
        utils::errorMsg("Failed creating new ServerMediaSession");
    }
    
    if (format == MPEGTS){
        subsession = MPEGTSQueueServerMediaSubsession::createNew(*env, False);
    }
}

RTSPConnection::~RTSPConnection()
{
    stopPlaying();
}

bool RTSPConnection::addVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId)
{
    if (!replicator){
        utils::errorMsg("Failed! Stream replicator is null!");
        return false;
    }
    
    for (auto it : getReaders()){
        if (readerId == it){
            utils::errorMsg("Failed! Readers can not be repeated in connection!");
            return false;
        }
    }
    
    if (format == MPEGTS){
        return addMPEGTSVideo(codec, replicator, readerId);
    } else {
        return addRawVideoSubsession(codec, replicator, readerId);
    }
}

bool RTSPConnection::addAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                                        unsigned channels, unsigned sampleRate, 
                                        SampleFmt sampleFormat, int readerId)
{
    if (!replicator){
        return false;
    }
    
    for (auto it : getReaders()){
        if (readerId == it){
            return false;
        }
    }
    
    if (format == MPEGTS){
        return addMPEGTSAudio(codec, replicator, readerId);
    } else {
        return addRawAudioSubsession(codec, replicator, channels, sampleRate, sampleFormat, readerId);
    }
}

bool RTSPConnection::addRawVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId)
{
    ServerMediaSubsession *sSession = NULL;
    switch(codec){
        case H264:
            sSession = H264QueueServerMediaSubsession::createNew(*fEnv, replicator, readerId, False);
            break;
        case H265:
            sSession = H265QueueServerMediaSubsession::createNew(*fEnv, replicator, readerId, False);
            break;
        case VP8:
            sSession =  VP8QueueServerMediaSubsession::createNew(*fEnv, replicator, readerId, False);
            break;
        default:
            break;
    }
    
    if (!sSession){
        utils::errorMsg("Failed! Could not create ServerMediaSubsession!");
        return false;
    }
    
    session->addSubsession(sSession);
    
    return true;
}

bool RTSPConnection::addRawAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                                        unsigned channels, unsigned sampleRate, 
                                        SampleFmt sampleFormat, int readerId)
{
    ServerMediaSubsession *sSession = NULL;
    switch(codec) {
        case AAC:
            sSession = ADTSQueueServerMediaSubsession::createNew(*fEnv, replicator, readerId, 
                                                                 channels, sampleRate, False);
            break;
        default:
            sSession = AudioQueueServerMediaSubsession::createNew(*fEnv, replicator,
                                                              readerId, codec, channels,
                                                              sampleRate, sampleFormat, False);
            break;
    }
    
    if (!sSession){
        return false;
    }
    
    session->addSubsession(sSession);
    
    return true;
}

bool RTSPConnection::addMPEGTSVideo(VCodecType codec, StreamReplicator* replicator, int readerId)
{
    if (!subsession){
        utils::errorMsg("MPEGTS subsession not defined!");
        return false;
    }
        
    if (!subsession->addVideoSource(codec, replicator, readerId)){
        utils::errorMsg("Failed adding video to MPEGTS subsession");
        return false;
    }
    
    if (!addedSub){
        session->addSubsession(subsession);
        addedSub = true;
    }
    
    return true;
}

bool RTSPConnection::addMPEGTSAudio(ACodecType codec, StreamReplicator* replicator, int readerId)
{
    if (!subsession){
        utils::errorMsg("MPEGTS subsession not defined!");
        return false;
    }
        
    if (!subsession->addAudioSource(codec, replicator, readerId)){
        utils::errorMsg("Failed adding audio to MPEGTS subsession");
        return false;
    }
    
    if (!addedSub){
        session->addSubsession(subsession);
        addedSub = true;
    }
    
    return true;
}

std::string RTSPConnection::getURI()
{
    if (rtspServer == NULL){
        return "";
    }
    
    if (session == NULL){
        return "";
    }

    return rtspServer->rtspURL(session);
}


bool RTSPConnection::startPlaying()
{
    if (started){
        return true;
    }
    
    if (rtspServer == NULL){
        return false;
    }
    
    if (session == NULL){
        return false;
    }

    rtspServer->addServerMediaSession(session);
    std::string url = rtspServer->rtspURL(session);

    utils::infoMsg("Play stream using the URL " + url);
    started = true;
    
    return true;
}

void RTSPConnection::stopPlaying()
{   
    if (session == NULL){
        return;
    }
    
    if (rtspServer == NULL){
        return;
    }

    rtspServer->deleteServerMediaSession(session);
    
    session = NULL;
    subsession = NULL;
    addedSub = false;
    started = false;
}

std::vector<int> RTSPConnection::getReaders()
{
    std::vector<int> readers;
    ServerMediaSubsessionIterator subIt(*session);
    ServerMediaSubsession *subsession;
    QueueServerMediaSubsession *qSubsession;
    
    subsession = subIt.next();
    while(subsession){
        if (!(qSubsession = dynamic_cast<QueueServerMediaSubsession *>(subsession))){
            utils::errorMsg("Could not cast to QueueServerMediaSubsession");
            return readers;
        }
        for (auto red : qSubsession->getReaderIds()){
            readers.push_back(red);
        }
        subsession = subIt.next();
    }
    return readers;
}

bool RTSPConnection::specificSetup() 
{
    //TODO:
    return true;
}

////////////////////
// RTP CONNECTION //
////////////////////

RTPConnection::RTPConnection(UsageEnvironment* env, FramedSource* source,
                             std::string ip, unsigned port) :
                             Connection(env), fIp(ip), fPort(port), 
                             rtcp(NULL), rtpGroupsock(NULL), rtcpGroupsock(NULL),   
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
                                 std::string ip, unsigned port, VCodecType codec, int readerId) : 
                                 RTPConnection(env, source, ip, port), fCodec(codec), reader(readerId)
{
    
}

bool VideoConnection::additionalSetup()
{
    switch(fCodec){
        case H264:
            fSink = H264VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            fSource = H264VideoStreamDiscreteFramer::createNew(*fEnv, fSource);
            break;
        case H265:
            fSink = H265VideoRTPSink::createNew(*fEnv, rtpGroupsock, 96);
            fSource = H265VideoStreamDiscreteFramer::createNew(*fEnv, fSource);
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

std::vector<int> VideoConnection::getReaders()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

AudioConnection::AudioConnection(UsageEnvironment* env, FramedSource *source,
                                 std::string ip, unsigned port, ACodecType codec,
                                 unsigned channels, unsigned sampleRate, 
                                 SampleFmt sampleFormat, int readerId) :
                                 RTPConnection(env, source, ip, port), fCodec(codec),
                                 fChannels(channels), fSampleRate(sampleRate),
                                 fSampleFormat(sampleFormat), reader(readerId)
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
    } else if (fCodec == AAC) {
        fSource = ADTSStreamParser::createNew(*fEnv, fSource);
        fSink = CustomMPEG4GenericRTPSink::createNew(*fEnv, rtpGroupsock, payloadType, 
                                                     fSampleRate, "audio", "AAC-hbr", fChannels);
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

std::vector<int> AudioConnection::getReaders()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

///////////////////////////////
// ULTRAGRID RTP CONNECTIONS //
///////////////////////////////

UltraGridVideoConnection::UltraGridVideoConnection(UsageEnvironment* env, FramedSource *source, 
                                                   std::string ip, unsigned port, VCodecType codec, int readerId) :
                                                   RTPConnection(env, source, ip, port), 
                                                   fCodec(codec), reader(readerId)
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

std::vector<int> UltraGridVideoConnection::getReaders()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

UltraGridAudioConnection::UltraGridAudioConnection(UsageEnvironment* env, FramedSource *source,
                                                   std::string ip, unsigned port, ACodecType codec,
                                                   unsigned channels, unsigned sampleRate,
                                                   SampleFmt sampleFormat, int readerId) :
                                RTPConnection(env, source, ip, port), fCodec(codec),
                                fChannels(channels), fSampleRate(sampleRate),
                                fSampleFormat(sampleFormat), reader(readerId)
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

std::vector<int> UltraGridAudioConnection::getReaders()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

////////////////////////
// MPEG-TS CONNECTION //
////////////////////////

MpegTsConnection::MpegTsConnection(UsageEnvironment* env, std::string ip, unsigned port) 
: RTPConnection(env, NULL, ip, port), audioReader(-1), videoReader(-1)
{
    tsFramer = MPEG2TransportStreamFromESSource::createNew(*env);
}

bool MpegTsConnection::addVideoSource(FramedSource* source, VCodecType codec, int readerId)
{
    FramedSource* startCodeInjector;
    
    if (codec != H264 && codec != H265) {
        utils::errorMsg("Error creating MPEG-TS Connection. Only H264 and H265 video codecs are valid");
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

    if (videoReader == -1){
        videoReader = readerId;
    } else {
        utils::errorMsg("Error video reader ID was already set.");
        return false;
    }
        
    startCodeInjector = H264or5StartCodeInjector::createNew(*fEnv, source, codec);
    if (codec == H264) tsFramer->addNewVideoSource(startCodeInjector, 5/*mpegVersion: H.264*/);
    if (codec == H265) tsFramer->addNewVideoSource(startCodeInjector, 6/*mpegVersion: H.265*/);

    return true;
}

bool MpegTsConnection::addAudioSource(FramedSource* source, ACodecType codec, int readerId)
{
    if (codec != AAC && codec != MP3) {
        utils::errorMsg("Error creating MPEG-TS Connection. Only AAC and MP3 audio codecs are valid");
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
    
    if (audioReader == -1){
        audioReader = readerId;
    } else {
        utils::errorMsg("Error video reader ID was already set.");
        return false;
    }

    if (codec == AAC) tsFramer->addNewAudioSource(source, 4/*mpegVersion: AAC*/);
    if (codec == MP3) tsFramer->addNewAudioSource(source, 1/*mpegVersion: MP3*/);
    
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

std::vector<int> MpegTsConnection::getReaders()
{
    std::vector<int> readers;
    if (audioReader != -1){
        readers.push_back(audioReader);
    }
    
    if (videoReader != -1){
        readers.push_back(videoReader);
    }
    
    return readers;
}
