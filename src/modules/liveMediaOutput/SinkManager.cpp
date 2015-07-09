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
 *            Marc Palau <marc.palau@i2cat.net>
 */


#include <GroupsockHelper.hh>

#include "SinkManager.hh"
#include "../../AVFramedQueue.hh"
#include "../../AudioCircularBuffer.hh"
#include "QueueSource.hh"
#include "H264or5QueueSource.hh"
#include "Types.hh"
#include "Connection.hh"
#include "../../Types.hh"
#include "../../Utils.hh"


SinkManager* SinkManager::createNew(unsigned readersNum)
{
    SinkManager *sMgr = new SinkManager(readersNum);
    
    if (sMgr->isGood()){
        return sMgr;
    }
    
    return NULL;
}

SinkManager::SinkManager(unsigned readersNum) :
TailFilter(SERVER, readersNum, true), rtspServer(NULL)
{
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    
    unsigned port = RTSP_PORT;

    while(rtspServer == NULL && port <= (RTSP_PORT + 10)){
        utils::infoMsg("Starting RTSP server at port: " + std::to_string(port));
        rtspServer = RTSPServer::createNew(*env, port, NULL);
        if (rtspServer == NULL) {
            utils::errorMsg("Failed to create RTSP server");
            port += 2;
        }
    }

    OutPacketBuffer::increaseMaxSizeTo(MAX_VIDEO_FRAME_SIZE);

    fType = TRANSMITTER;
    initializeEventMap();
}

SinkManager::~SinkManager()
{
    stop();
    delete scheduler;
    envir()->reclaim();
    env = NULL;
}

void SinkManager::stop()
{
    for (auto it : connections) {
       delete it.second;
    }
    connections.clear();

    for (auto it : replicators) {
        if (it.second->numReplicas() == 0){
            Medium::close(it.second);
            sources.erase(it.first);
        }
    }
    replicators.clear();
    
    for (auto it : sources) {
       Medium::close(it.second);
    }
    sources.clear();
    
    // NOTE: this crashes because rtsp sessions are destroyed inside each RTSP connection and 
    // Medium::close(rtspServer) seemms to do this internally, generating a segFault. Check deeper 
    // if not closing RTSP server is the best solution (port binding has been checked and it works
    // properly -- no port binding when stopping)
    if (rtspServer){
        Medium::close(rtspServer);
    }
}

bool SinkManager::readerInConnection(int rId)
{
    for (auto it : connections){
        for (auto id : it.second->getReaders()){
            if (id == rId){
                return true;
            }
        }
    }
    return false;
}

bool SinkManager::doProcessFrame(std::map<int, Frame*> oFrames)
{
    if (envir() == NULL){
        return false;
    }
    
    for (auto it: oFrames){
        if (it.second && it.second->getConsumed()){
            it.second->setConsumed(false);
            if (sources.count(it.first) > 0 && sources[it.first]->setFrame(it.second)){
                QueueSource::signalNewFrameData(scheduler, sources[it.first]);
            }
            
        }
    }

    scheduler->SingleStep();

    return true;
}

bool SinkManager::removeConnection(int id)
{
    Connection* connection;
    if (connections.count(id) <= 0){
        return false;
    }
    
    connection = connections[id];
    connections.erase(id);
    connection->stopPlaying();
    
    delete connection;
    
    return true;
}

bool SinkManager::addRTSPConnection(std::vector<int> readers, int id, TxFormat txformat, 
                                    std::string name, std::string info, std::string desc)
{
    RTSPConnection* connection;
    if (!rtspServer){
        utils::errorMsg("Unitialized RTSPServer");
        return false;
    }

    if (connections.count(id) > 0){
        utils::errorMsg("Failed, this connection already exists");
        return false;
    }

    connection = new RTSPConnection(env, txformat, rtspServer, name, info, desc);
    //TODO: test connection construction

    for (auto & reader : readers){
        if (!addSubsessionByReader(connection, reader)) {
            utils::errorMsg("Error adding subsession by reader in RTSP Connection");
            delete connection;
            return false;
        }
    }
    
    if (!connection->setup()) {
        utils::errorMsg("Error in RTSPConnection setup");
        delete connection;
        return false;
    }

    connections[id] = connection;
    return true;
}

bool SinkManager::addRTPConnection(std::vector<int> inputReaders, int id, std::string ip, int port, TxFormat txFormat)
{
    bool ret;
    
    if (connections.count(id) > 0) {
        utils::errorMsg("Error creating RTP connection. Specified ID already in use");
        return false;
    }

    for (auto iReader : inputReaders) {
        if (getReader(iReader) == NULL) {
            utils::errorMsg("Error creating RTP connection. Specified ID already in use");
            return false;
        }
    }
    
    switch (txFormat) {
        case STD_RTP:
            ret = addStdRTPConnection(inputReaders, id, ip, port);
            break;
        case ULTRAGRID:
            ret = addUltraGridRTPConnection(inputReaders, id, ip, port);
            break;
        case MPEGTS:
            ret = addMpegTsRTPConnection(inputReaders, id, ip, port);
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

bool SinkManager::addMpegTsRTPConnection(std::vector<int> inputReaders, int id, std::string ip, int port)
{
    VideoFrameQueue *vQueue = NULL;
    AudioFrameQueue *aQueue = NULL;
    MpegTsConnection* conn = NULL;
    bool success = false;
    bool hasVideo = false;
    bool hasAudio = false;

    if (inputReaders.size() <= 0 || inputReaders.size() > 2) {
        utils::errorMsg("Error in MPEG-TS RTP connection setup. Only 1 or 2 readers are supported");
        return false;
    }
    
    conn = new MpegTsConnection(envir(), ip, port);

    if (!conn) {
        utils::errorMsg("Error creating MpegTSRTPConnection");
        return false;
    }
    
    for (auto inReader : inputReaders) {

        vQueue = dynamic_cast<VideoFrameQueue*>(getReader(inReader)->getQueue());
        aQueue = dynamic_cast<AudioFrameQueue*>(getReader(inReader)->getQueue());
        
        if (replicators.count(inReader) <= 0){
            utils::errorMsg("Replicator is NULL for reader: " + std::to_string(inReader));
            continue;
        }

        if (vQueue && !hasVideo) {
            success = conn->addVideoSource(replicators[inReader]->createStreamReplica(), vQueue->getCodec(), inReader);
            hasVideo = true;
        } else if (aQueue && !hasAudio) {
            success = conn->addAudioSource(replicators[inReader]->createStreamReplica(), aQueue->getCodec(), inReader);
            hasAudio = true;
        } else {
            utils::errorMsg("Error creating MpegTSRTPConnection. Only one video and/or one audio is supported");
            success = false;
        }
    }

    if (!success) {
        utils::errorMsg("Error creating MpegTSRTPConnection. Readers not valid");
        return false;
    }

    if (!conn->setup()) {
        utils::errorMsg("Error in MpegTSRTPConnection setup");
        delete conn;
        return false;
    }

    connections[id] = conn;
    return true;
}

bool SinkManager::addStdRTPConnection(std::vector<int> readers, int id, std::string ip, int port)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Connection *conn = NULL;
    Reader *r;

    if (readers.size() != 1) {
        utils::errorMsg("Error in standard RTP connection setup. Multiple readers do not make sense in this type");
        return false;
    }

    r = getReader(readers.front());

    if (!r) {
        utils::errorMsg("Error in connection setup. Reader does not exist");
        return false;
    }

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(r->getQueue())) != NULL) {
        conn = new VideoConnection(envir(), replicators[readers.front()]->createStreamReplica(),
                                   ip, port, vQueue->getCodec(), readers.front());
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL){
        conn = new AudioConnection(envir(), replicators[readers.front()]->createStreamReplica(),
                                   ip, port, aQueue->getCodec(), aQueue->getChannels(),
                                   aQueue->getSampleRate(), aQueue->getSampleFmt(), readers.front());
    }

    if (!conn) {
        utils::errorMsg("Error creating StdRTPConnection");
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

bool SinkManager::addUltraGridRTPConnection(std::vector<int> readers, int id, std::string ip, int port)
{
    VideoFrameQueue *vQueue = NULL;
    AudioFrameQueue *aQueue = NULL;
    Connection* conn = NULL;
    Reader *r;

    if (readers.size() != 1) {
        utils::errorMsg("Error in standard Ultragrid connection setup. Multiple readers do not make sense in this type");
        return false;
    }

    r = getReader(readers.front());

    if (!r) {
        utils::errorMsg("Error in connection setup. Reader does not exist");
        return false;
    }

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(r->getQueue())) != NULL) {
        conn = new UltraGridVideoConnection(envir(), replicators[readers.front()]->createStreamReplica(), ip, port, vQueue->getCodec(), readers.front());
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL) {
        conn = new UltraGridAudioConnection(envir(), replicators[readers.front()]->createStreamReplica(), ip, port,
                        aQueue->getCodec(), aQueue->getChannels(), aQueue->getSampleRate(), aQueue->getSampleFmt(), readers.front());
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


Reader *SinkManager::setReader(int readerId, FrameQueue* queue)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    bool queueSrcCreated = false;

    if (readers.size() >= getMaxReaders() || readers.count(readerId) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader();
    

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(queue)) != NULL){
        createVideoQueueSource(vQueue->getCodec(), readerId);
        queueSrcCreated = true;
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(queue)) != NULL){
        createAudioQueueSource(aQueue->getCodec(), readerId);
        queueSrcCreated = true;
    }
    
    if(!queueSrcCreated){
        delete r;
        return NULL;
    }
    
    readers[readerId] = r;
    
    return r;
}

void SinkManager::createVideoQueueSource(VCodecType codec, int readerId)
{
    switch(codec){
        case H264:
        case H265:
            sources[readerId] = H264or5QueueSource::createNew(*(envir()), readerId);
            replicators[readerId] = StreamReplicator::createNew(*(envir()), sources[readerId], False);
            break;
        case VP8:
        default:
            sources[readerId] = QueueSource::createNew(*(envir()), readerId);
            replicators[readerId] =  StreamReplicator::createNew(*(envir()), sources[readerId], False);
            break;
    }
}

void SinkManager::createAudioQueueSource(ACodecType codec, int readerId)
{    
    sources[readerId] = QueueSource::createNew(*(envir()), readerId);
    replicators[readerId] = StreamReplicator::createNew(*(envir()), sources[readerId], False);
}

bool SinkManager::addSubsessionByReader(RTSPConnection* connection ,int readerId)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    
    Reader* reader = getReader(readerId);
    if (!reader){
        return false;
    }

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(reader->getQueue())) != NULL){
        return connection->addVideoSubsession(vQueue->getCodec(), replicators[readerId], readerId);
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(reader->getQueue())) != NULL){
        return connection->addAudioSubsession(aQueue->getCodec(), replicators[readerId], 
                                              aQueue->getChannels(), aQueue->getSampleRate(), 
                                              aQueue->getSampleFmt(), readerId);
    }

    return false;
}

//TODO: shall we keep it?
// bool SinkManager::removeConnectionByReaderId(int readerId)
// {
//     std::vector<int> readers;
//     std::vector<int> connToDelete;
//     bool ret = false;
//     Connection* conn;
//     
//     for (auto it : connections){
//         readers = it.second->getReaders();
//         for (auto ti : readers){
//             if (ti == readerId){
//                 it.second->stopPlaying();
//                 connToDelete.push_back(it.first);
//             }
//         }
//     }
//     
//     for (auto id : connToDelete){
//         conn = connections[id];
//         connections.erase(id);
//         delete conn;
//         ret |= true;
//     }
//     
//     return ret;
// }
// 
// bool SinkManager::deleteReader(int id)
// {
//     Reader* reader = getReader(id);
//     if (reader == NULL){
//         return false;
//     }
// 
//     removeConnectionByReaderId(id);
// 
//     if (reader->isConnected()) {
//         return false;
//     }
// 
//     delete reader;
//     readers.erase(id);
//     
//     if (sources.count(id) > 0 && sources[id]){
//         delete sources[id];
//         sources.erase(id);
//     }
// 
//     return true;
// }

void SinkManager::initializeEventMap()
{
    eventMap["addRTSPConnection"] = std::bind(&SinkManager::addRTSPConnectionEvent, this, std::placeholders::_1);
    eventMap["addRTPConnection"] = std::bind(&SinkManager::addRTPConnectionEvent, this, std::placeholders::_1);
}

bool SinkManager::addRTSPConnectionEvent(Jzon::Node* params)
{
    int id;
    TxFormat txFormat;
    std::string name, strTxFormat;
    std::string info = "";
    std::string desc = "";
    std::vector<int> readers;

    if (!params) {
        return false;
    }

    if (!params->Has("id") || !params->Has("txFormat") || !params->Has("name")) {
        return false;
    }

    if (!params->Has("readers") || !params->Get("readers").IsArray()) {
        return false;
    }

    id = params->Get("id").ToInt();
    name = params->Get("name").ToString();
    strTxFormat = params->Get("txFormat").ToString();
    txFormat = utils::getTxFormatFromString(strTxFormat);
    
    if (params->Has("info")) {
        info = params->Get("info").ToString();
    }
    
    if (params->Has("desc")) {
        desc = params->Get("desc").ToString();
    }

    Jzon::Array jsonReaders = params->Get("readers").AsArray();

    for (Jzon::Array::iterator it = jsonReaders.begin(); it != jsonReaders.end(); ++it) {
        readers.push_back((*it).ToInt());
    }

    if (readers.empty()) {
        return false;
    }

    return addRTSPConnection(readers, id, txFormat, name, info, desc);
}

bool SinkManager::addRTPConnectionEvent(Jzon::Node* params)
{
    std::vector<int> readers;
    int connectionId;
    std::string ip;
    int port;
    std::string stringTxFormat;
    TxFormat txFormat;

    if (!params) {
        return false;
    }

    if (!params->Has("id") || !params->Has("port") || !params->Has("ip") || !params->Has("txFormat")) {
        return false;
    }

    if (!params->Has("readers") || !params->Get("readers").IsArray()) {
        return false;
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
        return false;
    }

    return addRTPConnection(readers, connectionId, ip, port, txFormat);
}

void SinkManager::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array connectionArray;

    RTPConnection* rtpConn;
    RTSPConnection* rtspConn;
    std::string uri;

    for (auto it : connections) {
        Jzon::Array jsonReaders;
        Jzon::Object jsonConnection;
        
        if ((rtspConn = dynamic_cast<RTSPConnection*>(it.second))){
            jsonConnection.Add("name", rtspConn->getName());
            jsonConnection.Add("uri", rtspConn->getURI());
        } else if ((rtpConn = dynamic_cast<RTPConnection*>(it.second))){
            jsonConnection.Add("ip", rtpConn->getIP());
            jsonConnection.Add("port", std::to_string(rtpConn->getPort()));
        } else {
            filterNode.Add("error", Jzon::null);
        }

        for (auto reader : it.second->getReaders()){
            jsonReaders.Add(reader);
        }
        
        jsonConnection.Add("readers", jsonReaders);
        connectionArray.Add(jsonConnection);
    }

    filterNode.Add("sessions", connectionArray);
}
