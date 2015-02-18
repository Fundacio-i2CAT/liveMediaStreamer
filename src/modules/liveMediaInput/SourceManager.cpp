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
#define FILE_SINK_RECEIVE_BUFFER_SIZE 200000

FrameQueue* createVideoQueue(char const* codecName);
FrameQueue* createAudioQueue(unsigned char rtpPayloadFormat,
                             char const* codecName, unsigned channels,
                             unsigned sampleRate);

SourceManager::SourceManager(unsigned writersNum): LiveMediaFilter(0, writersNum), watch(0)
{
    fType = RECEIVER;

    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    initializeEventMap();
}

SourceManager::~SourceManager()
{
    for (auto it : sessionMap) {
        delete it.second;
    }

    delete &envir()->taskScheduler();
    envir()->reclaim();
    env = NULL;
}

void SourceManager::stop()
{
    watch = 1;
}

void SourceManager::setCallback(std::function<void(char const*, unsigned short)> callbackFunction)
{
    if (!callback) {
        callback = callbackFunction;
    }
}

bool SourceManager::hasCallback()
{
    if (callback) {
        return true;
    }

    return false;
}

bool SourceManager::runDoProcessFrame()
{
    if (envir() == NULL){
        return false;
    }

    envir()->taskScheduler().doEventLoop((char*) &watch);

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
    if (sessionMap.count(id) <= 0) {
        return false;
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

bool SourceManager::addWriter(unsigned port, const Writer *writer)
{
    if(writers.count(port) > 0){
        return false;
    }
    writers[port] = writer;

    return true;
}

FrameQueue *SourceManager::allocQueue(int wId)
{
    MediaSubsession *mSubsession;

    for (auto it : sessionMap) {
        if ((mSubsession = it.second->getSubsessionByPort(wId)) == NULL) {
            continue;
        }

        if (strcmp(mSubsession->mediumName(), "audio") == 0) {
            return createAudioQueue(mSubsession->rtpPayloadFormat(),
                mSubsession->codecName(), mSubsession->numChannels(),
                mSubsession->rtpTimestampFrequency());
        }

        if (strcmp(mSubsession->mediumName(), "video") == 0) {
            return createVideoQueue(mSubsession->codecName());
        }
    }

    return NULL;
}

void SourceManager::initializeEventMap()
{
    eventMap["addSession"] = std::bind(&SourceManager::addSessionEvent, this,
                                        std::placeholders::_1,  std::placeholders::_2);
}

void SourceManager::addSessionEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    std::string sessionId = utils::randomIdGenerator(ID_LENGTH);
    std::string sdp, medium, codec;
    int payload, bandwidth, timeStampFrequency, channels, port;
    Session* session;

    if (!params) {
        outputNode.Add("error", "Error adding session. Wrong parameters!");
        return;
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
                outputNode.Add("error", "Payload type is not valid!!");
                return;
            }

            sdp += makeSubsessionSDP(medium, PROTOCOL, payload, codec, bandwidth,
                                                timeStampFrequency, port, channels);
        }

        session = Session::createNew(*env, sdp, sessionId, this);

    } else {
        outputNode.Add("error", "Error adding session. Wrong parameters!");
        return;
    }

    if (addSession(session)) {
        session->initiateSession();
    }

    outputNode.Add("error", Jzon::null);
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


FrameQueue* createVideoQueue(char const* codecName)
{
    VCodecType codec;

    if (strcmp(codecName, "H264") == 0) {
        codec = H264;
    } else if (strcmp(codecName, "VP8") == 0) {
        codec = VP8;
    } else if (strcmp(codecName, "MJPEG") == 0) {
        codec = MJPEG;
    } else {
        return NULL;
    }

    return VideoFrameQueue::createNew(codec);
}

FrameQueue* createAudioQueue(unsigned char rtpPayloadFormat, char const* codecName, unsigned channels, unsigned sampleRate)
{
    ACodecType codec;
    //is this one neeeded? in should be implicit in PCMU case
    if (rtpPayloadFormat == 0) {
        codec = G711;
        return AudioFrameQueue::createNew(codec);
    }

    if (strcmp(codecName, "OPUS") == 0) {
        codec = OPUS;
        return AudioFrameQueue::createNew(codec, sampleRate);
    }

    if (strcmp(codecName, "MPEG4-GENERIC") == 0) {
        codec = AAC;
        return AudioFrameQueue::createNew(codec, sampleRate);
    }

    if (strcmp(codecName, "MPA") == 0) {
        codec = MP3;
        return AudioFrameQueue::createNew(codec, sampleRate);
    }

    if (strcmp(codecName, "PCMU") == 0) {
        codec = PCMU;
         return AudioFrameQueue::createNew(codec, sampleRate, channels);
    }

    if (strcmp(codecName, "PCM") == 0) {
        codec = PCM;
        return AudioFrameQueue::createNew(codec, sampleRate, channels);
    }

    //TODO: error msg codec not supported
    return NULL;
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
        delete[] newSession;
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
                if (!scs->addWriterToMngr(queueSink->getPort(), queueSink->getWriter())){
                    utils::errorMsg("Failed adding writer in SourceManager");
                    subsession->deInitiate();
                    return false;
                }
            }
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
    streamTimerTask(NULL), duration(0.0), id(id_)
{
}

StreamClientState::~StreamClientState() 
{
    delete iter;
    if (session != NULL) {

        UsageEnvironment& env = session->envir();

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}

bool StreamClientState::addWriterToMngr(unsigned port, Writer* writer)
{
    return mngr->addWriter(port, writer);
}