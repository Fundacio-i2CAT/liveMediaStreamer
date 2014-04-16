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
#include "ExtendedMediaSession.hh"
#include "ExtendedRTSPClient.hh"
#include <H264VideoFileSink.hh>

#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define FILE_SINK_RECEIVE_BUFFER_SIZE 200000

SourceManager *SourceManager::mngrInstance = NULL;

SourceManager::SourceManager():
    sessionList(HashTable::create(ONE_WORD_HASH_KEYS)), run(False), watch(0)
{    
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    this->env = BasicUsageEnvironment::createNew(*scheduler);
    
    
    mngrInstance = this;
}

SourceManager* 
SourceManager::getInstance(){
    if (mngrInstance != NULL){
        return mngrInstance;
    }
    srand(time(NULL));
    return new SourceManager();
}

void SourceManager::closeManager()
{
    //TODO:
}

void *startServer(void *args)
{
    char* watch = (char*) args;
    SourceManager* instance = SourceManager::getInstance();
    
    if (instance == NULL || instance->envir() == NULL){
        return NULL;
    }
    instance->envir()->taskScheduler().doEventLoop(watch); 
    
    instance->closeManager();
    delete &instance->envir()->taskScheduler();
    instance->envir()->reclaim();
    
    return NULL;
}

Boolean SourceManager::runManager()
{
    int ret;
    watch = 0;
    ret = pthread_create(&mngrTh, NULL, startServer, &watch);
    if (ret == 0){
        run = True;
    } else {
        run = False;
    }
    return run;
}

Boolean SourceManager::stopManager()
{
    watch = 1;
    if (run){
        pthread_join(mngrTh, NULL);
    }
    return True;
}

Boolean SourceManager::isRunning()
{
    return run;
}

Boolean SourceManager::addSession(char* id, char* mediaSessionType, char* sessionName, char* sessionDescription)
{   
    Session* session = Session::createNew(*(this->env), mediaSessionType, sessionName, sessionDescription);
    if (session == NULL){
        envir()->setResultMsg("Failed creating a new session Session");
        return False;
    }
    
    sessionList->Add(id, session);
    
    return True;
}

Boolean SourceManager::addRTSPSession(char* id, char const* progName, char const* rtspURL)
{   
    Session* session = Session::createNewByURL(*(this->env), progName, rtspURL);
    if (session == NULL){
        envir()->setResultMsg("Failed creating a new session Session");
        return False;
    }
    
    sessionList->Add(id, session);
    
    return True;
}

Boolean SourceManager::removeSession(char* id)
{
    Boolean closed;
    MediaSession* session;
    
    if ((session = (MediaSession *) sessionList->Lookup(id)) == NULL){
        return False;
    }
    
    //TODO: manage closure
    //closed = session->close();
    
    //if (closed){
    //    return sessionList->remove(id);
    //}
    
    return False;
}

Session* SourceManager::getSession(char* sessionId)
{
    Session* session;
    session = (Session*) sessionList->Lookup(sessionId);
    
    return session;
}

Boolean SourceManager::initiateAll()
{
    char const* id = (char *)malloc((ID_LENGTH+1)*sizeof(char));
    Boolean init = False;
    Session* session;
    HashTable::Iterator* iter = HashTable::Iterator::create(*sessionList);
    session = (Session *) iter->next(id);
    while (session != NULL){
        if (session->initiateSession()){
            init = True;
        }
        session = (Session *) iter->next(id);
    }
    return init;
}

void SourceManager::randomIdGenerator(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

// Implementation of "Session"

Session::Session()
  : session(NULL), client(NULL), iter(NULL) {
}

Session* Session::createNew(UsageEnvironment& env, char* mediaSessionType, char* sessionName, char* sessionDescription)
{    
    Session* newSession = new Session();
    ExtendedMediaSession* mSession = ExtendedMediaSession::createNew(env);
    
    if (mSession == NULL){
        return NULL;
    }
    
    mSession->setMediaSession(mediaSessionType, sessionName, sessionDescription);
    
    newSession->session = (MediaSession*) mSession;
    
    return newSession;
}

Session* Session::createNewByURL(UsageEnvironment& env, char const* progName, char const* rtspURL)
{
    Session* session = new Session();
    
    
    RTSPClient* rtspClient = ExtendedRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
    if (rtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return NULL;
    }
    
    session->client = rtspClient;
}

Boolean Session::initiateSession()
{
    MediaSubsession* subsession;
    
    if (this->session != NULL){
        UsageEnvironment& env = this->session->envir();
        this->iter = new MediaSubsessionIterator(*(this->session));
        subsession = this->iter->next();
        while (subsession != NULL) {
            if (!subsession->initiate()) {
                env << "Failed to initiate the subsession: " << env.getResultMsg() << "\n";
            } else if (!addSubsessionSink(env, subsession)){
                env << "Failed to initiate subsession sink\n";
                subsession->deInitiate();
            } else {
                env << "Initiated the subsession (client ports " << subsession->clientPortNum() << "-" << subsession->clientPortNum()+1 << ")\n";
            }
            subsession = this->iter->next();
        }
        return True;
    } else if (this->client != NULL){
        this->client->sendDescribeCommand(ExtendedRTSPClient::continueAfterDESCRIBE);
        return True;
    }
    
    return False;
}

Session::~Session() {
    //TODO: finalize RTSPClient if any and MediaSession
    delete iter;
}

void subsessionAfterPlaying(void* clientData) 
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;

    Medium::close(subsession->sink);
    subsession->sink = NULL;

    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; 
    }
}

void subsessionByeHandler(void* clientData) 
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    subsessionAfterPlaying(subsession);
}

Boolean Session::addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession)
{
    char fileName[100];
    strcpy(fileName,subsession->parentSession().sessionName());
    strcat(fileName, subsession->mediumName());
    strcat(fileName, ".");
    strcat(fileName, subsession->codecName());
    
    //TODO: this should use or default queueSink
    if (strcmp(subsession->mediumName(), "audio") == 0) {
        subsession->sink = FileSink::createNew(env, 
                                               fileName, 
                                               FILE_SINK_RECEIVE_BUFFER_SIZE, False);
    } else if (strcmp(subsession->mediumName(), "video") == 0 && 
        strcmp(subsession->codecName(), "H264") == 0) {
        subsession->sink = H264VideoFileSink::createNew(env, 
                                               fileName,
                                               subsession->fmtp_spropparametersets(), FILE_SINK_RECEIVE_BUFFER_SIZE, False);
    }
    
    if (subsession->sink == NULL){
        return False;
    }
    
    subsession->sink->startPlaying(*(subsession->readSource()),
                       subsessionAfterPlaying, subsession);
    
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (subsession->rtcpInstance() != NULL) {
        subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, subsession);
    }
    
    return True;
}


