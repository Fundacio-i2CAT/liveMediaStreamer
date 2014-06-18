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


#include "SinkManager.hh"
#include "../../AVFramedQueue.hh"
#include "../../AudioCircularBuffer.hh"
#include "H264QueueServerMediaSubsession.hh"
#include "VP8QueueServerMediaSubsession.hh"
#include "AudioQueueServerMediaSubsession.hh"


SinkManager *SinkManager::mngrInstance = NULL;

SinkManager::SinkManager(int readersNum): watch(0), TailFilter(readersNum)
{    
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    this->env = BasicUsageEnvironment::createNew(*scheduler);
    
    //TODO: Add authentication security
    rtspServer = RTSPServer::createNew(*env, RTSP_PORT, NULL);
    if (rtspServer == NULL) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    }
    
    OutPacketBuffer::increaseMaxSizeTo(MAX_VIDEO_FRAME_SIZE);
    
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
    watch = 1;
    if (mngrTh.joinable()){
        mngrTh.join();
    }
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


bool SinkManager::isRunning()
{
    return mngrTh.joinable();
}

bool SinkManager::addSession(std::string id, std::vector<int> readers, std::string info, std::string desc)
{
    if (sessionList.count(id) > 0){
        return false;
    }
    
    ServerMediaSession* servSession
        = ServerMediaSession::createNew(*(envir()), id.c_str(), info.c_str(), desc.c_str());
        
    if (servSession == NULL){
        return false;
    }
    
    ServerMediaSubsession *subsession;
    
    for (auto & reader : readers){
        if ((subsession = createSubsessionByReader(getReader(reader))) != NULL){
            servSession->addSubsession(subsession);
        } else {
            //TODO: delete ServerMediaSession and previous subsessions
            return false;
        }
    }
    
    sessionList[id] = servSession;
    
    return true;
}

ServerMediaSubsession *SinkManager::createVideoMediaSubsession(VCodecType codec, Reader *reader)
{
    switch(codec){
        case H264:
            return H264QueueServerMediaSubsession::createNew(*(envir()), reader, True);
            break;
        case VP8:
            return VP8QueueServerMediaSubsession::createNew(*(envir()), reader, True);
            break;
        default:
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createAudioMediaSubsession(ACodecType codec, Reader *reader)
{
    switch(codec){
        case AAC:
            //TODO
            break;
        default:
            return AudioQueueServerMediaSubsession::createNew(*(envir()), reader, True);
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createSubsessionByReader(Reader *reader)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    AudioCircularBuffer *circularBuffer;
    
    if ((vQueue = dynamic_cast<VideoFrameQueue*>(reader->getQueue())) != NULL){
        return createVideoMediaSubsession(vQueue->getCodec(), reader);
    }
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(reader->getQueue())) != NULL){
        return createAudioMediaSubsession(aQueue->getCodec(), reader);
    }
    if ((circularBuffer = dynamic_cast<AudioCircularBuffer*>(reader->getQueue())) != NULL){
        //TODO
    }
    return NULL;
}

bool SinkManager::removeSession(std::string id)
{   
    if (sessionList.find(id) == sessionList.end()){
        envir()->setResultMsg("Failed, no session found with this id!\n");
        return false;
    }
   
    //TODO: manage closure, should add RTSP reference?
    
    //RTSPServer::closeAllClientSessionsForServerMediaSession()
    //session->deleteAllSubsessions()
    
    sessionList.erase(id);
    
    return false;
}

bool SinkManager::publishSession(std::string id)
{  
    if (sessionList.find(id) == sessionList.end()){
        envir()->setResultMsg("Failed, no session found with this id!\n");
        return false;
    }
    
    if (rtspServer == NULL){
        return false;
    }
    
    rtspServer->addServerMediaSession(sessionList[id]);
    char* url = rtspServer->rtspURL(sessionList[id]);
    UsageEnvironment& env = rtspServer->envir();
    
    env << "\n\nPlay this stream using the URL \"" << url << "\"\n";
    delete[] url;
    
    return true;
}

ServerMediaSession* SinkManager::getSession(std::string id)
{
    if(sessionList.find(id) != sessionList.end()){
        return sessionList[id];
    }
    
    return NULL;
}


