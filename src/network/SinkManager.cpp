/*
 *  SinkManager.cpp - Class that handles multiple server sessions dynamically.
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

#ifndef _SINK_MANAGER_HH
#include "SinkManager.hh"
#endif

SinkManager *SinkManager::mngrInstance = NULL;

SinkManager::SinkManager():
    sessionList(HashTable::create(ONE_WORD_HASH_KEYS)), watch(0)
{    
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    this->env = BasicUsageEnvironment::createNew(*scheduler);
    
    //TODO: Add authentication security
    RTSPServer* rtspServer = RTSPServer::createNew(*env, RTSP_PORT, NULL);
    if (rtspServer == NULL) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    }
    
    mngrInstance = this;
}

SinkManager* 
SinkManager::getInstance(){
    if (mngrInstance != NULL){
        return mngrInstance;
    }
    srand(time(NULL));
    return new SinkManager();
}

void SinkManager::closeManager()
{
    //TODO:
}

void SinkManager::destroyInstance()
{
    //TODO:
}

void *startServer(void *args)
{
    char* watch = (char*) args;
    SinkManager* instance = SinkManager::getInstance();
    
    if (instance == NULL || instance->envir() == NULL){
        return NULL;
    }
    instance->envir()->taskScheduler().doEventLoop(watch); 
    
    instance->closeManager();
    delete &instance->envir()->taskScheduler();
    instance->envir()->reclaim();
    
    return NULL;
}

bool SinkManager::runManager()
{
    watch = 0;
    mngrTh = std::thread(std::bind(startServer, &watch));
    return mngrTh.joinable();
}

bool SinkManager::stopManager()
{
    watch = 1;
    if (mngrTh.joinable()){
        mngrTh.join();
    }
    return true;
}

bool SinkManager::isRunning()
{
    return mngrTh.joinable();
}

bool SinkManager::addSession(char* id, char const* streamName, 
                                char const* info, char const* description)
{   
    ServerMediaSession* session = 
        ServerMediaSession::createNew(*env, streamName, info, description);
        
    if (session == NULL){
        envir()->setResultMsg("Failed creating a new server session");
        return false;
    }
    
    sessionList->Add(id, session);
    
    return true;
}

bool SinkManager::removeSession(char* id)
{
    ServerMediaSession* session;
    
    if ((session = (ServerMediaSession *) sessionList->Lookup(id)) == NULL){
        return false;
    }
    
    //TODO: manage closure, should add RTSP reference?
    
    //RTSPServer::closeAllClientSessionsForServerMediaSession()
    //session->deleteAllSubsessions()
    
    return false;
}

bool SinkManager::publishSession(char* id)
{
    ServerMediaSession* session;
    
    if ((session = (ServerMediaSession *) sessionList->Lookup(id)) == NULL){
        return false;
    }
    
    if (rtspServer == NULL){
        return false;
    }
    
    rtspServer->addServerMediaSession(session);
    
    return true;
}

ServerMediaSession* SinkManager::getSession(char* sessionId)
{
    ServerMediaSession* session;
    session = (ServerMediaSession*) sessionList->Lookup(sessionId);
    
    return session;
}


void SinkManager::randomIdGenerator(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

