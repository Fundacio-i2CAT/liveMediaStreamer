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
#include "Handlers.hh"
#include "QueueSink.hh"

#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define FILE_SINK_RECEIVE_BUFFER_SIZE 200000


SourceManager *SourceManager::mngrInstance = NULL;

SourceManager::SourceManager(): watch(0)
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

bool SourceManager::addSession(std::string id, Session* session)
{   
    if (sessionList.find(id) != sessionList.end()){
        envir()->setResultMsg("Failed adding session! Duplicated id!\n");
        return false;
    }
    sessionList[id] = session;
    
    return true;
}



bool SourceManager::removeSession(std::string id)
{   
    if (sessionList.find(id) == sessionList.end()){
        envir()->setResultMsg("Failed, no session with this id!\n");
        return false;
    }
    
    sessionList.erase(id);
    
    return true;
}

Session* SourceManager::getSession(std::string id)
{
    if(sessionList.find(id) != sessionList.end()){
        return sessionList[id];
    }
        
    return NULL;
}

bool SourceManager::initiateAll()
{
    bool init = false;
    std::map<std::string,Session*>::iterator it;
    for (it=sessionList.begin(); it!=sessionList.end(); ++it){
        if (it->second->initiateSession()){
            init = true;
        }
    }
    
    return init;
}

void SourceManager::addFrameQueue(FrameQueue* queue)
{
    inputs.push_back(queue);
}

void SourceManager::removeFrameQueue(FrameQueue* queue)
{
    inputs.remove(queue);
}

// Implementation of "Session"

Session::Session()
  : session(NULL), client(NULL), iter(NULL) {
}

Session* Session::createNew(UsageEnvironment& env, std::string sdp)
{    
    Session* newSession = new Session();
    MediaSession* mSession = MediaSession::createNew(env, sdp.c_str());
    
    if (mSession == NULL){
        delete[] newSession;
        return NULL;
    }
    
    newSession->session = mSession;
    
    return newSession;
}

Session* Session::createNewByURL(UsageEnvironment& env, std::string progName, std::string rtspURL)
{
    Session* session = new Session();
    
    
    RTSPClient* rtspClient = ExtendedRTSPClient::createNew(env, rtspURL.c_str(), RTSP_CLIENT_VERBOSITY_LEVEL, progName.c_str());
    if (rtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL.c_str() << "\": " << env.getResultMsg() << "\n";
        return NULL;
    }
    
    session->client = rtspClient;
}

bool Session::initiateSession()
{
    MediaSubsession* subsession;
    
    if (this->session != NULL){
        UsageEnvironment& env = this->session->envir();
        this->iter = new MediaSubsessionIterator(*(this->session));
        subsession = this->iter->next();
        while (subsession != NULL) {
            if (!subsession->initiate()) {
                env << "Failed to initiate the subsession: " << env.getResultMsg() << "\n";
            } else if (!handlers::addSubsessionSink(env, subsession)){
                env << "Failed to initiate subsession sink\n";
                subsession->deInitiate();
            } else {
                env << "Initiated the subsession (client ports " << subsession->clientPortNum() << "-" << subsession->clientPortNum()+1 << ")\n";
            }
            subsession = this->iter->next();
        }
        return true;
    } else if (this->client != NULL){
        this->client->sendDescribeCommand(handlers::continueAfterDESCRIBE);
        return true;
    }
    
    return false;
}

Session::~Session() {
    MediaSubsession* subsession;
    this->iter = new MediaSubsessionIterator(*(this->session));
    subsession = this->iter->next();
    while (subsession != NULL) {
        subsession->deInitiate();
        Medium::close(subsession->sink);
        subsession = this->iter->next();
    }
    Medium::close(this->session);
    delete[] iter;
    
    if (client != NULL) {
        Medium::close(client);
    }
}





