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
//TODO: safe incldues!!
#include "ExtendedRTSPClient.hh"


#ifndef _AV_FRAMED_QUEUE_HH
#include "../AVFramedQueue.hh"
#endif

#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define FILE_SINK_RECEIVE_BUFFER_SIZE 200000

SourceManager *SourceManager::mngrInstance = NULL;


FrameQueue* createVideoQueue(char const* codecName);
FrameQueue* createAudioQueue(unsigned char rtpPayloadFormat, 
                             char const* codecName, unsigned channels, 
                             unsigned sampleRate);

SourceManager::SourceManager(int writersNum): watch(0), HeadFilter(writersNum)
{    
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    this->env = BasicUsageEnvironment::createNew(*scheduler);
    
    mngrInstance = this;
}

SourceManager* SourceManager::getInstance(){
    if (mngrInstance != NULL){
        return mngrInstance;
    }
    
    return new SourceManager();
}

void SourceManager::closeManager()
{
    sessionList.clear();
    if (this->isRunning()){
        watch = 1;
        if (mngrTh.joinable()){
            mngrTh.join();
        }
    }
}

void SourceManager::destroyInstance()
{
    SourceManager * mngrInstance = SourceManager::getInstance();
    if (mngrInstance != NULL){
        mngrInstance->closeManager();
        delete[] mngrInstance;
        mngrInstance = NULL;
    }
}

void* SourceManager::startServer(void *args)
{
    char* watch = (char*) args;
    SourceManager* instance = SourceManager::getInstance();
    
    if (instance == NULL || instance->envir() == NULL){
        return NULL;
    }
    instance->envir()->taskScheduler().doEventLoop(watch); 
    
    delete &instance->envir()->taskScheduler();
    instance->envir()->reclaim();
    
    return NULL;
}

bool SourceManager::runManager()
{
    watch = 0;
    mngrTh = std::thread(std::bind(SourceManager::startServer, &watch));
    return mngrTh.joinable();
}

bool SourceManager::isRunning()
{
    return mngrTh.joinable();
}

bool SourceManager::addSession(Session* session)
{   
    if (session == NULL){
        return false;
    }
    if (getSession(session->getId()) != NULL){
        return false;
    }
    sessionList.push_back(session);
    
    return true;
}

bool SourceManager::removeSession(std::string id)
{
    std::list<Session*>::iterator it;
    for (it=sessionList.begin(); it!=sessionList.end(); ++it){
        if ((*it)->getId() == id){
            sessionList.erase(it);
            return true;
        }
    }
    return false;
}

Session* SourceManager::getSession(std::string id)
{
    std::list<Session*>::iterator it;
    for (it=sessionList.begin(); it!=sessionList.end(); ++it){
        if ((*it)->getId() == id){
            return *it;
        }
    }
    return NULL;
}

void SourceManager::addConnection(int wId, MediaSubsession* subsession)
{
    connection_t connection;

    connection.rtpPayloadFormat = subsession->rtpPayloadFormat();
    connection.codecName = subsession->codecName();
    connection.channels = subsession->numChannels();
    connection.sampleRate = subsession->rtpTimestampFrequency();
    connection.port = subsession->clientPortNum();
    connection.session = subsession->parentSession().sessionName();
    connection.medium = subsession->mediumName();
    
    if (strcmp(subsession->mediumName(), "audio") == 0) {
        audioOutputs[wId] = connection;
    } else if (strcmp(subsession->mediumName(), "video") == 0) {
        videoOutputs[wId] = connection;
    }

    //Todo: callback
}

FrameQueue *SourceManager::allocQueue(int wId)
{
    for (auto it : audioOutputs){
        if (it.first == wId){
            connection_t connection = audioOutputs[wId];
            return createAudioQueue(connection.rtpPayloadFormat,
                connection.codecName, connection.channels,
                connection.sampleRate);
        }
    }
    for (auto it : videoOutputs){
        if (it.first == wId){
            connection_t connection = videoOutputs[wId];
            return createVideoQueue(connection.codecName);
        }
    }
    return NULL;
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
        //TODO: codec not supported
    }
    
    return VideoFrameQueue::createNew(codec, 0);
}

FrameQueue* createAudioQueue(unsigned char rtpPayloadFormat, char const* codecName, unsigned channels, unsigned sampleRate)
{
    ACodecType codec;
    
    if (rtpPayloadFormat == 0) {
        codec = G711;
        return AudioFrameQueue::createNew(codec, 0);
    }
    
    if (strcmp(codecName, "OPUS") == 0) {
        codec = OPUS;
        return AudioFrameQueue::createNew(codec, 0, sampleRate);
    }
    
    if (strcmp(codecName, "PCMU") == 0) {
        codec = PCMU;
        return AudioFrameQueue::createNew(codec, 0, sampleRate, channels);
    }
    
    //TODO: error msg codec not supported
    return NULL;
}

// Implementation of "Session"

Session::Session(std::string id)
  : client(NULL)
{
    scs = new StreamClientState(id);
}

Session* Session::createNew(UsageEnvironment& env, std::string sdp, std::string id)
{    
    Session* newSession = new Session(id);
    MediaSession* mSession = MediaSession::createNew(env, sdp.c_str());
    
    if (mSession == NULL){
        delete[] newSession;
        return NULL;
    }
    
    newSession->scs->session = mSession;
    
    return newSession;
}

Session* Session::createNewByURL(UsageEnvironment& env, std::string progName, std::string rtspURL, std::string id)
{
    Session* session = new Session(id);
    
    
    RTSPClient* rtspClient = ExtendedRTSPClient::createNew(env, rtspURL.c_str(), session->scs, RTSP_CLIENT_VERBOSITY_LEVEL, progName.c_str());
    if (rtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL.c_str() << "\": " << env.getResultMsg() << "\n";
        return NULL;
    }
    
    session->client = rtspClient;
    
    return session;
}

bool Session::initiateSession()
{
    MediaSubsession* subsession;
    
    if (this->scs->session != NULL){
        UsageEnvironment& env = this->scs->session->envir();
        this->scs->iter = new MediaSubsessionIterator(*(this->scs->session));
        subsession = this->scs->iter->next();
        while (subsession != NULL) {
            if (!subsession->initiate()) {
                env << "Failed to initiate the subsession: " << env.getResultMsg() << "\n";
            } else if (!handlers::addSubsessionSink(env, subsession)){
                env << "Failed to initiate subsession sink\n";
                subsession->deInitiate();
            } else {
                env << "Initiated the subsession (client ports " << subsession->clientPortNum() << "-" << subsession->clientPortNum()+1 << ")\n";
            }
            subsession = this->scs->iter->next();
        }
        return true;
    } else if (client != NULL){
        client->sendDescribeCommand(handlers::continueAfterDESCRIBE);
        return true;
    }
    
    return false;
}

Session::~Session() {
    MediaSubsession* subsession;
    this->scs->iter = new MediaSubsessionIterator(*(this->scs->session));
    subsession = this->scs->iter->next();
    while (subsession != NULL) {
        subsession->deInitiate();
        Medium::close(subsession->sink);
        subsession = this->scs->iter->next();
    }
    Medium::close(this->scs->session);
    delete[] this->scs->iter;
    
    if (client != NULL) {
        Medium::close(client);
    }
}

// Implementation of "StreamClientState":

StreamClientState::StreamClientState(std::string id_)
: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
    id = id_;
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        
        UsageEnvironment& env = session->envir(); 
        
        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}
