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
#include "QueueSource.hh"
#include "H264QueueSource.hh"
#include "Types.hh"
#include "Connection.hh"
#include "../../Types.hh"
#include "../../Utils.hh"


SinkManager *SinkManager::mngrInstance = NULL;

SinkManager::SinkManager(int readersNum): TailFilter(readersNum), watch(0)
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

SinkManager::~SinkManager()
{
    delete &envir()->taskScheduler();
    envir()->reclaim();
    env = NULL;
    
    mngrInstance = NULL;
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
    for (auto it : sessionList ) {
        rtspServer->deleteServerMediaSession(it.second);
    }
    
    for (auto it : replicators) {
       Medium::close(it.second);
    }
    
    for (auto it : connections) {
       delete it.second;
    }

    Medium::close(rtspServer);

    watch = 1;
}

void SinkManager::destroyInstance()
{
    //TODO:
}

bool SinkManager::processFrame(bool removeFrame)
{   
    if (envir() == NULL){
        return false;
    }
    
    envir()->taskScheduler().doEventLoop((char*) &watch);

    return true;
}

bool SinkManager::addSession(std::string id, std::vector<int> readers, std::string info, std::string desc)
{
    if (!rtspServer){
        utils::errorMsg("Unitialized RTSPServer");
        return false;
    }

    if (sessionList.count(id) > 0){
        utils::errorMsg("Failed, this session already exists: " + id);
        return false;
    }
    
    ServerMediaSession* servSession
        = ServerMediaSession::createNew(*(envir()), id.c_str(), info.c_str(), desc.c_str());
        
    if (servSession == NULL){
        utils::errorMsg("Failed creating new ServerMediaSession");
        return false;
    }
    
    ServerMediaSubsession *subsession;
    
    for (auto & reader : readers){
        if ((subsession = createSubsessionByReader(reader)) != NULL) {
            servSession->addSubsession(subsession);
        } else {
            servSession->deleteAllSubsessions();
            Medium::close(servSession);
            return false;
        }
    }
    
    sessionList[id] = servSession;
    
    return true;
}

bool SinkManager::addRTPConnection(std::vector<int> readersId, int id, std::string ip,  int port, TxFormat txFmt)
{
    bool success = false;

    for (auto r : readersId) {
        if (readers.count(r) < 1){
            utils::errorMsg("Invalid reader Id");
            return success;
        }
    }

    if (connections.count(id) > 0){
        utils::errorMsg("Connection id must be unique");
        return success;
    }

    switch(txFmt) {
        case STD_RTP:
            success = addStdRTPConnection(readersId.front(), id, ip, port);
        break;
        case ULTRAGRID:
            success = addUltraGridRTPConnection(readersId.front(), id , ip ,port);
        break;
        case MPEGTS:
            // NOTE: in this case more than one reader can be attached to MPEGTS because it muxes the data.
            // This is the reason why readersId is a vector<int>, not an int
            utils::errorMsg("Error creating connections. MPEGTS Transport format still not supported");
        break;
        default:
            utils::errorMsg("Error creating connections. Transport format not supported");
        break;
    }

    return success;
}

bool SinkManager::addDASHConnection(int reader, unsigned id, std::string fileName, 
                                    std::string quality, bool reInit, uint32_t segmentTime, 
                                    uint32_t initSegment, uint32_t fps)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Connection* conn = NULL;

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(getReader(reader)->getQueue())) != NULL) {
        conn = new DashVideoConnection(envir(), replicators[reader]->createStreamReplica(), 
                                       fileName, quality, reInit, segmentTime, initSegment,
                                       vQueue->getCodec(), fps);

    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(getReader(reader)->getQueue())) != NULL) {
        conn = new DashAudioConnection(envir(), replicators[reader]->createStreamReplica(), 
                                       fileName, quality, reInit, segmentTime, initSegment,
                                       aQueue->getCodec(), aQueue->getChannels(),
                                       aQueue->getSampleRate(), aQueue->getSampleFmt());
    }

    if (!conn) {
        utils::errorMsg("Error creating RawRTPConnection");
        return false;
    }

    if (!conn->setup()) {
        utils::errorMsg("Error in connection setup");
        delete conn;
        return false;
    }

    connections[id] = conn;
    return true;
}

bool SinkManager::addStdRTPConnection(int reader, int id, std::string ip, int port)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Connection *conn = NULL;

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(getReader(reader)->getQueue())) != NULL) {
        conn = new VideoConnection(envir(), replicators[reader]->createStreamReplica(), 
                                   ip, port, vQueue->getCodec());
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(getReader(reader)->getQueue())) != NULL){ 
        conn = new AudioConnection(envir(), replicators[reader]->createStreamReplica(), 
                                   ip, port, aQueue->getCodec(), aQueue->getChannels(),
                                   aQueue->getSampleRate(), aQueue->getSampleFmt());
    }

    if (!conn) {
        utils::errorMsg("Error creating RawRTPConnection");
        return false;
    }

    if (!conn->setup()) {
        utils::errorMsg("Error in connection setup");
        delete conn;
        return false;
    }

    connections[id] = conn;
    return true;
}

bool SinkManager::addUltraGridRTPConnection(int reader, int id, std::string ip, int port)
{
    VideoFrameQueue *vQueue = NULL;
    AudioFrameQueue *aQueue = NULL;
    Connection* conn = NULL;

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(getReader(reader)->getQueue())) != NULL) {
        conn = new UltraGridVideoConnection(envir(), replicators[reader]->createStreamReplica(), ip, port);
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(getReader(reader)->getQueue())) != NULL) {
        conn = new UltraGridAudioConnection(envir(), replicators[reader]->createStreamReplica(), ip, port,
                        aQueue->getCodec(), aQueue->getChannels(), aQueue->getSampleRate(), aQueue->getSampleFmt());
    }

    if (!conn) {
        utils::errorMsg("Error creating RawRTPConnection");
        return false;
    }

    if (!conn->setup()) {
        utils::errorMsg("Error in connection setup");
        delete conn;
        return false;
    }

    connections[id] = conn;
    return true;
}


Reader *SinkManager::setReader(int readerId, FrameQueue* queue, bool sharedQueue)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    
    if (readers.size() >= getMaxReaders() || readers.count(readerId) > 0 ) {
        return NULL;
    }
    
    Reader* r = new Reader();
    readers[readerId] = r;
    
    if ((vQueue = dynamic_cast<VideoFrameQueue*>(queue)) != NULL){
        createVideoQueueSource(vQueue->getCodec(), r, readerId);
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(queue)) != NULL){
        createAudioQueueSource(aQueue->getCodec(), r, readerId);
    }
    
    return r;
}

void SinkManager::createVideoQueueSource(VCodecType codec, Reader *reader, int readerId)
{
    FramedSource *source;
    switch(codec){
        case H264:
            source = H264QueueSource::createNew(*(envir()), reader, readerId);
            replicators[readerId] = StreamReplicator::createNew(*(envir()), source, False);
            break;
        case VP8:
        default:
            source = QueueSource::createNew(*(envir()), reader, readerId);
            replicators[readerId] =  StreamReplicator::createNew(*(envir()), source, False);
            break;
    }
}

void SinkManager::createAudioQueueSource(ACodecType codec, Reader *reader, int readerId)
{
    QueueSource *source;
    switch(codec){
       // case MPEG4_GENERIC:
			//printf("TODO createAudioQueueSource\n");
            //TODO
            //break;
        default:
            source = QueueSource::createNew(*(envir()), reader, readerId);
            replicators[readerId] = StreamReplicator::createNew(*(envir()), source, False);
            break;
    }
}

ServerMediaSubsession *SinkManager::createVideoMediaSubsession(VCodecType codec, int readerId)
{
    switch(codec){
        case H264:
            return H264QueueServerMediaSubsession::createNew(*(envir()), replicators[readerId], readerId, False);
            break;
        case VP8:
            return VP8QueueServerMediaSubsession::createNew(*(envir()), replicators[readerId], readerId, False);
            break;
        default:
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createAudioMediaSubsession(ACodecType codec,
                                                               unsigned channels,
                                                               unsigned sampleRate,
                                                               SampleFmt sampleFormat, 
                                                               int readerId)
{
    switch(codec){
        //case MPEG4_GENERIC:
            //TODO
			//printf("TODO createAudioMediaSubsession\n");
            //break;
        default:
            return AudioQueueServerMediaSubsession::createNew(*(envir()), replicators[readerId], 
                                                              readerId, codec, channels,
                                                              sampleRate, sampleFormat, False);
            break;
    }
    return NULL;
}

ServerMediaSubsession *SinkManager::createSubsessionByReader(int readerId)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    
    if ((vQueue = dynamic_cast<VideoFrameQueue*>(getReader(readerId)->getQueue())) != NULL){
        return createVideoMediaSubsession(vQueue->getCodec(), readerId);
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(getReader(readerId)->getQueue())) != NULL){
        return createAudioMediaSubsession(aQueue->getCodec(), aQueue->getChannels(),
                                          aQueue->getSampleRate(), aQueue->getSampleFmt(), readerId);
    }

    return NULL;
}

bool SinkManager::removeSession(std::string id)
{   
    if (sessionList.count(id) <= 0){
        utils::errorMsg("Failed, no session found with this id (" + id + ")");
        return false;
    }

    rtspServer->deleteServerMediaSession(sessionList[id]);
    sessionList.erase(id);
    
    return false;
}

bool SinkManager::removeSessionByReaderId(int readerId)
{
    bool ret = false;
    QueueServerMediaSubsession *qSubsession;
    ServerMediaSubsession *subsession;
    
    std::map<std::string, ServerMediaSession*>::iterator it = sessionList.begin();
    while(it != sessionList.end()){
        ServerMediaSubsessionIterator subIt(*it->second);
        subsession = subIt.next();
        while(subsession){
            if (!(qSubsession = dynamic_cast<QueueServerMediaSubsession *>(subsession))){
                utils::errorMsg("Could not cast to QueueServerMediaSubsession");
                return false;
            }
            if (qSubsession->getReaderId() == readerId){
                std::map<std::string, ServerMediaSession*>::iterator tmpIt = it;
                removeSession(tmpIt->first);
                ret = true;
                break;
            }
            subsession = subIt.next();
        }
        it++;
    }
    return ret;
}

bool SinkManager::deleteReader(int id)
{
    Reader* reader = getReader(id);
    if (reader == NULL){
        return false;
    }
    
    removeSessionByReaderId(id);
    
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
    eventMap["addRTPConnection"] = std::bind(&SinkManager::addRTPConnectionEvent, this, 
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

    if (!publishSession(sessionId)){
        outputNode.Add("error", "Error adding session. Internal error!");
        return;
    }

    outputNode.Add("sessionID", sessionId);
}

void SinkManager::addRTPConnectionEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    std::vector<int> readers;
    int connectionId;
    std::string ip;
    int port;
    std::string stringTxFormat;
    TxFormat txFormat;

    if (!params) {
        outputNode.Add("error", "Error adding session. No parameters!");
        return;
    }

    if (!params->Has("id") || !params->Has("port") || !params->Has("ip") || !params->Has("txFormat")) {
        outputNode.Add("error", "Error adding connection. Wrong parameters!");
        return;
    }

    if (!params->Has("readers") || !params->Get("readers").IsArray()) {
        outputNode.Add("error", "Error adding connection. Readers does not exist or is not an array!");
        return;
    }

    connectionId = params->Get("id").ToInt();
    port = params->Get("port").ToInt();
    ip = params->Get("ip").ToString();
    stringTxFormat = params->Get("txFormat").ToString();
    txFormat = utils::getTxFormatFromString(stringTxFormat);

    Jzon::Array jsonReaders = params->Get("readers").AsArray();
    
    for (Jzon::Array::iterator it = jsonReaders.begin(); it != jsonReaders.end(); ++it) {
        readers.push_back((*it).ToInt());
    }
    
    if (readers.empty()) {
        outputNode.Add("error", "Error adding session. Readers array is empty!");
        return;
    }

    if(!addRTPConnection(readers, connectionId, ip, port, txFormat)) {
        outputNode.Add("error", "Error adding session. Internal error!");
        return;
    }
    outputNode.Add("error", Jzon::null);
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
