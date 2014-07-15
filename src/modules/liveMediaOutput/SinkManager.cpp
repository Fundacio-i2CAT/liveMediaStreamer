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


#include <GroupsockHelper.hh>

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
        utils::errorMsg("Failed to create RTSP server");
    }
    
    OutPacketBuffer::increaseMaxSizeTo(MAX_VIDEO_FRAME_SIZE);
    
    mngrInstance = this;
    fType = TRANSMITTER;
    initializeEventMap();

}

SinkManager* 
SinkManager::getInstance(){
    if (mngrInstance != NULL){
        return mngrInstance;
    }
    return new SinkManager();
}

void SinkManager::stop()
{
    watch = 1;
}

void SinkManager::destroyInstance()
{
    //TODO:
}

bool SinkManager::processFrame(bool removeFrame)
{
    SinkManager* instance = SinkManager::getInstance();
    
    if (envir() == NULL){
        return false;
    }
    envir()->taskScheduler().doEventLoop((char*) &watch); 
    
    delete &envir()->taskScheduler();
    envir()->reclaim();
    
    return true;
}

bool SinkManager::addSession(std::string id, std::vector<int> readers, std::string info, std::string desc)
{
    if (sessionList.count(id) > 0){
        utils::errorMsg("Failed, this session already exists: " + id);
        return false;
    }
    
    for(auto it : readers){
        if (activeReaders.count(it) > 0){
            utils::errorMsg("Readers already included in other sessions");
            return false;
        }
    }
    
    ServerMediaSession* servSession
        = ServerMediaSession::createNew(*(envir()), id.c_str(), info.c_str(), desc.c_str());
        
    if (servSession == NULL){
        utils::errorMsg("Failed creating new ServerMediaSession");
        return false;
    }
    
    ServerMediaSubsession *subsession;
    
    for (auto & reader : readers){
        if ((subsession = createSubsessionByReader(getReader(reader), reader)) != NULL) {
            servSession->addSubsession(subsession);
        } else {
            servSession->deleteAllSubsessions();
            Medium::close(servSession);
            return false;
        }
    }
    
    sessionList[id] = servSession;
    
    for(auto it : readers){
        activeReaders[it] = id;
    }
    
    return true;
}

bool SinkManager::addConnection(int reader, std::string ip, unsigned int port){
    ServerMediaSubsession* subsession;
    QueueServerMediaSubsession* qSubsession;
    ServerMediaSession* session;
    void* streamState = NULL;
    netAddressBits addr = our_inet_addr(ip.c_str());
    netAddressBits dstAddr;
    u_int32_t clientSessionId;
    unsigned short rtpSeqNum = 0;
    unsigned rtpTimestamp = 0;
    uint8_t destinationTTL = 255;
    Boolean multicast = False;
    Port serverRTPPort(0);
    Port serverRTCPPort(0);
    
    if ((port % 2) != 0){
        utils::errorMsg("Port must be an even number");
        return false;
    }
    
    if (activeReaders.count(reader) <= 0){
        utils::errorMsg("Reader is not active, must have an associated session");
        return false;
    }
    
    session = sessionList[activeReaders[reader]];
    ServerMediaSubsessionIterator subIt(*session);
    subsession = subIt.next();
    
    while(subsession){
        if (!(qSubsession = dynamic_cast<QueueServerMediaSubsession *>(subsession))){
            utils::errorMsg("Could not cast to QueueServerMediaSubsession");
            return false;
        }
        if (qSubsession->getReaderId() == reader){
            break;
        }
        subsession = subIt.next();
    }
    
    do {
        clientSessionId = our_random32();
    } while (qSubsession->hasDestinationClient(clientSessionId));
    
    subsession->getStreamParameters(clientSessionId, addr, 
                                    port, port + 1, -1, 0, 0, dstAddr, destinationTTL, 
                                    multicast, serverRTPPort, serverRTCPPort, streamState);
    
    if (!qSubsession->hasDestinationClient(clientSessionId) || streamState == NULL){
        utils::infoMsg("Could not getStreamParameters");
        return false;
    } 
    
    subsession->startStream(clientSessionId, 
                            streamState, NULL, NULL, rtpSeqNum, rtpTimestamp, NULL, NULL);
    
    connections[clientSessionId].first = reader;
    connections[clientSessionId].second = (StreamState*) streamState;
    
    return true;
}

ServerMediaSubsession *SinkManager::createVideoMediaSubsession(VCodecType codec, Reader *reader, int readerId)
{
    switch(codec){
        case H264:
            return H264QueueServerMediaSubsession::createNew(*(envir()), reader, readerId, True);
            break;
        case VP8:
            return VP8QueueServerMediaSubsession::createNew(*(envir()), reader, readerId, True);
            break;
        default:
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createAudioMediaSubsession(ACodecType codec, Reader *reader, int readerId)
{
    switch(codec){
        case AAC:
            //TODO
            break;
        default:
            return AudioQueueServerMediaSubsession::createNew(*(envir()), reader, readerId, True);
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createSubsessionByReader(Reader *reader, int readerId)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    AudioCircularBuffer *circularBuffer;
    
    if ((vQueue = dynamic_cast<VideoFrameQueue*>(reader->getQueue())) != NULL){
        return createVideoMediaSubsession(vQueue->getCodec(), reader, readerId);
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(reader->getQueue())) != NULL){
        return createAudioMediaSubsession(aQueue->getCodec(), reader, readerId);
    }

    if ((circularBuffer = dynamic_cast<AudioCircularBuffer*>(reader->getQueue())) != NULL){
        //TODO
    }

    return NULL;
}

bool SinkManager::removeSession(std::string id)
{   
    if (sessionList.count(id) <= 0){
        utils::errorMsg("Failed, no session found with this id (" + id + ")");
        return false;
    }
    
    for( std::map<int, std::string>::iterator i = activeReaders.begin(); i != activeReaders.end(); ) {
        if(i->second.compare(id) == 0) {
            activeReaders.erase( i++ ); 
        } else {
            ++i;
        }
    }
    
    rtspServer->closeAllClientSessionsForServerMediaSession(sessionList[id]);
    sessionList[id]->deleteAllSubsessions();
    Medium::close(sessionList[id]);
    
    sessionList.erase(id);
    
    return false;
}

bool SinkManager::deleteReader(int id)
{
    Reader* reader = getReader(id);
    if (reader == NULL){
        return false;
    }
    
    if (activeReaders.count(id) > 0){
        removeSession(activeReaders[id]);
    }
    
    if (reader->isConnected()) {
        return false;
    }
    
    delete reader;
    readers.erase(id);
    
    return true;
}

bool SinkManager::publishSession(std::string id)
{  
    if (sessionList.find(id) == sessionList.end()){
        utils::errorMsg("Failed, no session found with this id (" + id + ")");
        return false;
    }
    
    if (rtspServer == NULL){
        return false;
    }
    
    rtspServer->addServerMediaSession(sessionList[id]);
    char* url = rtspServer->rtspURL(sessionList[id]);
    UsageEnvironment& env = rtspServer->envir();
    
    utils::infoMsg("Play " + id + " stream using the URL " + url);
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

void SinkManager::initializeEventMap() 
{
     eventMap["addSession"] = std::bind(&SinkManager::addSessionEvent, this, 
                                        std::placeholders::_1,  std::placeholders::_2);
}

void SinkManager::addSessionEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    std::vector<int> readers;
    std::string sessionId;

    if (!params) {
        outputNode.Add("error", "Error adding session. No parameters!");
        return;
    }

    if (params->Has("sessionName")) {
        sessionId = params->Get("sessionName").ToString();
    } else {
        sessionId = utils::randomIdGenerator(ID_LENGTH);
    }

    if (!params->Has("readers") || !params->Get("readers").IsArray()) {
        outputNode.Add("error", "Error adding session. Readers does not exist or is not an array!");
        return;
    }
        
    Jzon::Array jsonReaders = params->Get("readers").AsArray();
    
    for (Jzon::Array::iterator it = jsonReaders.begin(); it != jsonReaders.end(); ++it) {
        readers.push_back((*it).ToInt());
    }
    
    if (readers.empty()) {
        outputNode.Add("error", "Error adding session. Readers array is empty!");
        return;
    }

    if(!addSession(sessionId, readers)) {
        outputNode.Add("error", "Error adding session. Internal error!");
        return;
    }

    publishSession(sessionId);

    outputNode.Add("sessionID", sessionId);
} 

void SinkManager::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array sessionArray;
    ServerMediaSubsession* subsession;
    int readerId;

    for (auto it : sessionList) {
        Jzon::Array jsonReaders;
        Jzon::Object jsonSession;
        std::string uri = rtspServer->rtspURL(it.second);

        jsonSession.Add("id", it.first);
        jsonSession.Add("uri", uri);

        ServerMediaSubsessionIterator sIt(*it.second);
        subsession = sIt.next();

        while(subsession) {
            readerId = dynamic_cast<QueueServerMediaSubsession*>(subsession)->getReaderId();
            jsonReaders.Add(readerId);
            subsession = sIt.next();
        }

        jsonSession.Add("readers", jsonReaders);
        sessionArray.Add(jsonSession);
    }

    filterNode.Add("sessions", sessionArray);
}
