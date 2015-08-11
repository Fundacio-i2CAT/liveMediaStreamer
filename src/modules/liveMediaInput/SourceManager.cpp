/*
 *  SourceManager.cpp - Class that handles multiple sessions dynamically.
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

#include "SourceManager.hh"
#include "ExtendedRTSPClient.hh"
#include "../../AVFramedQueue.hh"
#include "../../Utils.hh"

#include <sstream>

#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define RTP_RECEIVE_BUFFER_SIZE 2000000

static StreamInfo *createStreamInfo(const MediaSubsession *mss)
{
    StreamInfo *si = NULL;
    const char *codecName = mss->codecName();

    if (strcmp(mss->mediumName(), "audio") == 0) {
        si = new StreamInfo(AUDIO);
        if (mss->rtpPayloadFormat() == 0) {
            // Is this one neeeded? it should be implicit in PCMU case
            si->audio.codec = G711;
        } else
        if (strcmp(codecName, "OPUS") == 0) {
            si->audio.codec = OPUS;
        } else
        if (strcmp(codecName, "MPEG4-GENERIC") == 0) {
            si->audio.codec = AAC;
        } else
        if (strcmp(codecName, "PCMU") == 0) {
            si->audio.codec = PCMU;
        } else
        if (strcmp(codecName, "PCM") == 0) {
            si->audio.codec = PCM;
        } else {
            utils::errorMsg (std::string("Unsupported audio codec ") + codecName);
            delete si;
            return NULL;
        }
        si->setCodecDefaults();
        si->audio.sampleRate = mss->rtpTimestampFrequency();
        si->audio.channels = mss->numChannels();
    } else
    if (strcmp(mss->mediumName(), "video") == 0) {
        si = new StreamInfo(VIDEO);
        if (strcmp(codecName, "H264") == 0) {
            si->video.codec = H264;
            si->video.h264or5.annexb = true;
        } else if (strcmp(codecName, "H265") == 0) {
            si->video.codec = H265;
            si->video.h264or5.annexb = true;
        } else if (strcmp(codecName, "VP8") == 0) {
            si->video.codec = VP8;
        } else if (strcmp(codecName, "MJPEG") == 0) {
            si->video.codec = MJPEG;
        } else {
            utils::errorMsg (std::string("Unsupported video codec ") + codecName);
            delete si;
            return NULL;
        }
        si->setCodecDefaults();
    }
    return si;
}

SourceManager::SourceManager(unsigned writersNum): HeadFilter(writersNum, SERVER)
{
    fType = RECEIVER;

    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    initializeEventMap();
}

SourceManager::~SourceManager()
{
    for (auto it : sessionMap) {
        delete it.second;
    }

    delete scheduler;
    envir()->reclaim();
    env = NULL;

    for (auto it : outputStreamInfos) {
        delete it.second;
    }
}

bool SourceManager::doProcessFrame(std::map<int, Frame*> &dFrames)
{
    if (envir() == NULL){
        return false;
    }
    
    for (auto it : dFrames){
        if (sinks.count(it.first) > 0){
            sinks[it.first]->setFrame(it.second);
        }
    }
    
    scheduler->SingleStep();

    return true;
}

bool SourceManager::addSession(Session* session)
{
    if (session == NULL) {
        return false;
    }

    if (sessionMap.count(session->getId()) > 0) {
        return false;
    }

    sessionMap[session->getId()] = session;

    return true;
}

bool SourceManager::removeSession(std::string id)
{
    MediaSubsession *subsession;
    
    if (sessionMap.count(id) <= 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> guard(sinksMtx);
    
    for (auto it : sessionMap) {
        it.second->getScs()->iter = new MediaSubsessionIterator(*(it.second->getScs()->session));
        subsession = it.second->getScs()->iter->next();
        while (subsession != NULL) {
            if (sinks.count(subsession->clientPortNum()) > 0){
                Medium::close(sinks[subsession->clientPortNum()]);
                sinks.erase(subsession->clientPortNum());
                disconnectWriter(subsession->clientPortNum());
                it.second->getScs()->removeSubsessionStats(subsession->clientPortNum());
            }
            subsession = it.second->getScs()->iter->next();
        }
    }

    delete sessionMap[id];
    sessionMap.erase(id);

    return true;
}

Session* SourceManager::getSession(std::string id)
{
    if (sessionMap.count(id) <= 0) {
        return NULL;
    }

    return sessionMap[id];
}

bool SourceManager::addSink(unsigned port, QueueSink *sink)
{
    std::lock_guard<std::mutex> guard(sinksMtx);
    if(sinks.count(port) > 0){
        utils::warningMsg("sink id must be unique!");
        return false;
    }
    
    if (!sink){
        utils::warningMsg("sink is NULL, it has not been added!");
        return false;
    }
    
    sinks[port] = sink;

    return true;
}

FrameQueue *SourceManager::allocQueue(ConnectionData cData)
{
    MediaSubsession *mSubsession;
    StreamInfo *si = NULL;

    // Do we already have a StreamInfo for this writerId?
    if (outputStreamInfos.count(cData.writerId) > 0) {
        si = outputStreamInfos[cData.writerId];
    } else {
        for (auto it : sessionMap) {
            mSubsession = it.second->getSubsessionByPort(cData.writerId);
            if (mSubsession != NULL) {
                si = createStreamInfo (mSubsession);
                outputStreamInfos[cData.writerId] = si;
                break;
            }
        }
    }

    if (!si) {
        utils::errorMsg (std::string("Unknown port number ") + std::to_string(cData.writerId));
        return NULL;
    }
    if (si->type == AUDIO) {
        return AudioFrameQueue::createNew(cData, si, DEFAULT_AUDIO_FRAMES);
    }
    if (si->type == VIDEO) {
        return VideoFrameQueue::createNew(cData, si, DEFAULT_VIDEO_FRAMES);
    }
    return NULL;
}

void SourceManager::initializeEventMap()
{
    eventMap["addSession"] = std::bind(&SourceManager::addSessionEvent, this, std::placeholders::_1);
    eventMap["removeSession"] = std::bind(&SourceManager::removeSessionEvent, this, std::placeholders::_1);
}

bool SourceManager::removeSessionEvent(Jzon::Node* params)
{
    std::string sessionId;
    
    if (params->Has("id")) {
        sessionId = params->Get("id").ToString();
        return removeSession(sessionId);
    } 
    
    return false;
}

bool SourceManager::addSessionEvent(Jzon::Node* params)
{
    std::string sessionId = utils::randomIdGenerator(ID_LENGTH);
    std::string sdp, medium, codec;
    int payload, bandwidth, timeStampFrequency, channels, port;
    Session* session;

    if (!params) {
        return false;
    }

    if (params->Has("uri") && params->Has("progName") && params->Has("id")) {

        std::string progName = params->Get("progName").ToString();
        std::string rtspURL = params->Get("uri").ToString();
        sessionId = params->Get("id").ToString();
        session = Session::createNewByURL(*env, progName, rtspURL, sessionId, this);

    } else if (params->Has("subsessions") && params->Get("subsessions").IsArray()) {

        Jzon::Array subsessions = params->Get("subsessions").AsArray();
        sdp = makeSessionSDP(sessionId, "this is a test");

        for (Jzon::Array::iterator it = subsessions.begin(); it != subsessions.end(); ++it) {
            medium = (*it).Get("medium").ToString();
            codec = (*it).Get("codec").ToString();
            bandwidth = (*it).Get("bandwidth").ToInt();
            timeStampFrequency = (*it).Get("timeStampFrequency").ToInt();
            port = (*it).Get("port").ToInt();
            channels = (*it).Get("channels").ToInt();

            payload = utils::getPayloadFromCodec(codec);

            if (payload < 0) {
                return false;
            }

            sdp += makeSubsessionSDP(medium, PROTOCOL, payload, codec, bandwidth,
                                                timeStampFrequency, port, channels);
        }

        session = Session::createNew(*env, sdp, sessionId, this);

    } else {
        return false;
    }

    if (addSession(session)) {
        if(session->initiateSession()){
           return true; 
        } else {
            return false;
        } 
    } else {
        return false;
    }
}

std::string SourceManager::makeSessionSDP(std::string sessionName, std::string sessionDescription)
{
    std::stringstream sdp;
    sdp << "v=0\n";
    sdp << "o=- 0 0 IN IP4 127.0.0.1\n";
    sdp << "s=" << sessionName << "\n";
    sdp << "i=" << sessionDescription << "\n";
    sdp << "t= 0 0\n";

    return sdp.str();
}

std::string SourceManager::makeSubsessionSDP(std::string mediumName, std::string protocolName,
                              unsigned int RTPPayloadFormat,
                              std::string codecName, unsigned int bandwidth,
                              unsigned int RTPTimestampFrequency,
                              unsigned int clientPortNum,
                              unsigned int channels)
{
    std::stringstream sdp;
    sdp << "m=" << mediumName << " " << clientPortNum;
    sdp << " RTP/AVP " << RTPPayloadFormat << "\n";
    sdp << "c=IN IP4 127.0.0.1\n";
    sdp << "b=AS:" << bandwidth << "\n";

    if (RTPPayloadFormat < 96) {
        return sdp.str();
    }

    sdp << "a=rtpmap:" << RTPPayloadFormat << " ";
    sdp << codecName << "/" << RTPTimestampFrequency;
    if (channels != 0) {
        sdp << "/" << channels;
    }
    sdp << "\n";
    
    if (codecName.compare("H264") == 0){
        sdp << "a=fmtp:" << RTPPayloadFormat << " packetization-mode=1\n";
    }

    if (codecName.compare("MPEG4-GENERIC") == 0 && mediumName.compare("audio") == 0) {
        sdp << "a=fmtp:" << RTPPayloadFormat << " streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3\n";
    }

    return sdp.str();
}

void SourceManager::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array sessionArray;
    MediaSubsession* subsession;

    for (auto it : sessionMap) {
        Jzon::Array subsessionArray;
        Jzon::Object jsonSession;

        if (!it.second->getScs()->session) {
            continue;
        }

        MediaSubsessionIterator iter(*(it.second->getScs()->session));

        while ((subsession = iter.next()) != NULL) {
            Jzon::Object jsonSubsession;

            jsonSubsession.Add("port", subsession->clientPortNum());
            jsonSubsession.Add("medium", subsession->mediumName());
            jsonSubsession.Add("codec", subsession->codecName());

            subsessionArray.Add(jsonSubsession);
        }

        jsonSession.Add("id", it.first);
        jsonSession.Add("subsessions", subsessionArray);

        sessionArray.Add(jsonSession);
    }

    filterNode.Add("sessions", sessionArray);
}

// Implementation of "Session"

Session::Session(std::string id, SourceManager *const mngr)
  : client(NULL)
{
    scs = new StreamClientState(id, mngr);
}

Session* Session::createNew(UsageEnvironment& env, std::string sdp, std::string id, SourceManager *const mngr)
{
    Session* newSession = new Session(id, mngr);
    MediaSession* mSession = MediaSession::createNew(env, sdp.c_str());

    if (mSession == NULL){
        delete newSession;
        return NULL;
    }

    newSession->scs->session = mSession;

    return newSession;
}

Session* Session::createNewByURL(UsageEnvironment& env, std::string progName, std::string rtspURL, std::string id, SourceManager *const mngr)
{
    Session* session = new Session(id, mngr);

    RTSPClient* rtspClient = ExtendedRTSPClient::createNew(env, rtspURL.c_str(), session->scs, RTSP_CLIENT_VERBOSITY_LEVEL, progName.c_str());
    if (rtspClient == NULL) {
        utils::errorMsg("Failed to create a RTSP client for URL " + rtspURL);
        return NULL;
    }

    session->client = rtspClient;

    return session;
}

bool Session::initiateSession()
{
    MediaSubsession* subsession;
    QueueSink *queueSink;

    if (scs->session != NULL){
        UsageEnvironment& env = scs->session->envir();
        scs->iter = new MediaSubsessionIterator(*(scs->session));
        subsession = scs->iter->next();
        while (subsession != NULL) {
            if (!subsession->initiate()) {
                utils::errorMsg("Failed to initiate the subsession");
            } else if (!handlers::addSubsessionSink(env, subsession)){
                utils::errorMsg("Failed to initiate subsession sink");
                subsession->deInitiate();
            } else {
                utils::infoMsg("Initiated subsession at port: " +
                std::to_string(subsession->clientPortNum()));
                if(!(queueSink = dynamic_cast<QueueSink *>(subsession->sink))){
                    utils::errorMsg("Failed to initiate subsession sink");
                    subsession->deInitiate();
                    return false;
                }
                if (!scs->addSinkToMngr(queueSink->getPort(), queueSink)){
                    utils::errorMsg("Failed adding sink in SourceManager");
                    subsession->deInitiate();
                    return false;
                }
                if(!scs->addNewSubsessionStats(queueSink->getPort(), subsession)){
                    utils::errorMsg("Failed adding subsession statistics in SourceManager");
                    subsession->deInitiate();
                    return false;                    
                }
            }
	   
            increaseReceiveBufferTo(env, subsession->rtpSource()->RTPgs()->socketNum(), RTP_RECEIVE_BUFFER_SIZE);

           subsession = scs->iter->next();
        }
        return true;

    } else if (client != NULL){
        unsigned ret = client->sendDescribeCommand(handlers::continueAfterDESCRIBE);
        std::cout << "SEND DESCRIBE COMMAND RETURN: " << ret << std::endl;
        return true;
    }

    return false;
}

Session::~Session() {
    MediaSubsession* subsession;
    this->scs->iter = new MediaSubsessionIterator(*(this->scs->session));
    subsession = this->scs->iter->next();

    while (subsession != NULL) {
        Medium::close(subsession->sink);
        subsession = this->scs->iter->next();
    }

    Medium::close(this->scs->session);
    delete this->scs->iter;

    if (client != NULL) {
        Medium::close(client);
    }
}

MediaSubsession* Session::getSubsessionByPort(int port)
{
    MediaSubsession* subsession;

    MediaSubsessionIterator iter(*(this->scs->session));

    while ((subsession = iter.next()) != NULL) {
        if (subsession->clientPortNum() == port) {
            return subsession;
        }
    }

    return NULL;
}

// Implementation of "StreamClientState":

StreamClientState::StreamClientState(std::string id_, SourceManager *const  manager) :
    mngr(manager), iter(NULL), session(NULL), subsession(NULL),
    streamTimerTask(NULL), duration(0.0), sessionTimeoutBrokenServerTask(NULL),
    sendKeepAlivesToBrokenServers(True), // Send periodic 'keep-alive' requests to keep broken server sessions alive
    sessionTimeoutParameter(0), id(id_)
{
}

StreamClientState::~StreamClientState()
{
    delete iter;
    if (session != NULL) {

        UsageEnvironment& env = session->envir();

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        env.taskScheduler().unscheduleDelayedTask(sessionTimeoutBrokenServerTask);
        Medium::close(session);

        for (auto it : smsStats) {
            delete it.second;
        }
    }
}

bool StreamClientState::addSinkToMngr(unsigned id, QueueSink* sink)
{
    return mngr->addSink(id, sink);
}

bool StreamClientState::addNewSubsessionStats(size_t port, MediaSubsession* subsession)
{
    if (smsStats.count(port) > 0) {
        return false;
    }

    smsStats[port] = new SourceManagerSubsessionStats(subsession);

    smsStats[port]->beginStatsMeasurement();

    return true;    
}

bool StreamClientState::removeSubsessionStats(size_t port)
{
    if (smsStats.count(port) <= 0) {
        utils::errorMsg("Failed removing subsession stats in SourceManager");
        return false;
    }
    
    delete smsStats[port];
    smsStats.erase(port);

    return true;
}

SourceManagerSubsessionStats* StreamClientState::getSubsessionStats(size_t port)
{
    if (smsStats.count(port) <= 0) {
        utils::errorMsg("No subsession stats with id " +std::to_string(port) +" in SourceManager");
        return NULL;
    }

    return smsStats[port];
}

// Implementation of "SourceManagerSubsessionStats" class:

SourceManagerSubsessionStats::SourceManagerSubsessionStats(MediaSubsession* subsession) :
    port(subsession->clientPortNum()), avgPacketLoss(0), maxPacketLoss(0), minPacketLoss(0), 
    avgBitRate(0), maxBitRate(0), minBitRate(0),
    avgInterPacketGap(0), maxInterPacketGap(0), minInterPacketGap(0),
    fSubSession(subsession), nextStatsMeasurementUSecs(0), statsRecordHead(NULL)
{
}

SourceManagerSubsessionStats::~SourceManagerSubsessionStats()
{
}

void SourceManagerSubsessionStats::beginStatsMeasurement() {
    struct timeval startTime;
    gettimeofday(&startTime, NULL);
    nextStatsMeasurementUSecs = startTime.tv_sec*1000000 + startTime.tv_usec;
    statsMeasurements* statsRecordTail = NULL;

    if(fSubSession == NULL) return;

    RTPSource* src = fSubSession->rtpSource();
    if (src == NULL) return;

    statsMeasurements* statsRecord = new statsMeasurements(startTime, src);

    if (statsRecordHead == NULL) {
        statsRecordHead = statsRecord;
    }
    if (statsRecordTail != NULL) statsRecordTail->fNext = statsRecord;
    statsRecordTail  = statsRecord;

    //scheduleNextStatsMeasurement();
}
