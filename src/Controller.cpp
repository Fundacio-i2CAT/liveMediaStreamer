/*
 *  Controller.cpp - Controller class for livemediastreamer framework
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 *            David Cassany <david.cassany@i2cat.net>
 *			  Martin German <martin.german@i2cat.net>
 */

#include "Controller.hh"
#include "Utils.hh"
#include "modules/audioEncoder/AudioEncoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"
#include "modules/audioMixer/AudioMixer.hh"
#include "modules/videoEncoder/VideoEncoderX264.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/videoMixer/VideoMixer.hh"

Controller* Controller::ctrlInstance = NULL;
PipelineManager* PipelineManager::pipeMngrInstance = NULL;

void sendAndClose(Jzon::Object outputNode, int socket);

Controller::Controller()
{    
    ctrlInstance = this;
    pipeMngrInstance = PipelineManager::getInstance();
    inputRootNode = new Jzon::Object();
    parser = new Jzon::Parser(*inputRootNode);
    initializeEventMap();
    runFlag = true;
}

Controller* Controller::getInstance()
{
    if (ctrlInstance != NULL){
        return ctrlInstance;
    }
    
    return new Controller();
}

void Controller::destroyInstance()
{
    Controller * ctrlInstance = Controller::getInstance();
    if (ctrlInstance != NULL) {
        delete ctrlInstance;
        ctrlInstance = NULL;
    }
}

PipelineManager* Controller::pipelineManager()
{
    return pipeMngrInstance;
}


bool Controller::createSocket(int port)
{

    struct sockaddr_in serv_addr;
    int yes=1;

    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (listeningSocket < 0) {
        utils::errorMsg("Opening socket");
        return false;
    }
     
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
        utils::errorMsg("Setting socket options");
        return false;
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    if (bind(listeningSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        utils::errorMsg("On binding");
        return false;
    } 

    return true;
}

bool Controller::listenSocket()
{
    socklen_t clilen;
    struct sockaddr cli_addr;
    listen(listeningSocket,5);
    clilen = sizeof(cli_addr);

    connectionSocket = accept(listeningSocket, (struct sockaddr *) &cli_addr, &clilen);

    if (connectionSocket < 0) {
        return false;
    }

    return true;
}

bool Controller::readAndParse()
{
    bzero(inBuffer, MSG_BUFFER_MAX_LENGTH);
    inputRootNode->Clear();

    if (read(connectionSocket, inBuffer, MSG_BUFFER_MAX_LENGTH - 1) < 0) {
        utils::errorMsg("Reading from socket");
        return false;
    }

    parser->SetJson(inBuffer);

    if (!parser->Parse()) {
        //TODO: error
        return false;
    }

    return true;
}

void Controller::processRequest() 
{
    Jzon::Object outputNode;

    if (inputRootNode->Get("events").IsArray()) {
        const Jzon::Array &events = inputRootNode->Get("events").AsArray();

        if (!processEventArray(events)) {
            outputNode.Add("error", "Error while processing event array...");
        } else {
            outputNode.Add("error", Jzon::null);
        }
        
        sendAndClose(outputNode, connectionSocket);

    } else if (inputRootNode->Get("events").IsObject()) {
        processEvent(inputRootNode->Get("events"), connectionSocket);

    } else {
        outputNode.Add("error", "Error while processing event. INvalid JSON format...");
        sendAndClose(outputNode, connectionSocket);
    }
    
}

bool Controller::processEventArray(const Jzon::Array events)
{
    for (Jzon::Array::const_iterator it = events.begin(); it != events.end(); ++it) {
        if(!processEvent((*it), -1)) {
            return false;
        }
    }

    return true;
}

bool Controller::processEvent(Jzon::Object event, int socket) 
{
    if (event.Get("filterID").ToInt() != 0) {
        return processFilterEvent(event, socket);
    }
 
    return processInternalEvent(event, socket);
}

bool Controller::processFilterEvent(Jzon::Object event, int socket) 
{
    int filterID = -1;
    BaseFilter *filter = NULL;
    Jzon::Object outputNode;

    if (!event.Has("action") || !event.Has("params")) {
        outputNode.Add("error", "Error while processing event. Invalid JSON format...");
        sendAndClose(outputNode, socket);
        return false;
    }

    filterID = event.Get("filterID").ToInt();
    filter = pipeMngrInstance->getFilter(filterID);

    if (!filter) {
        outputNode.Add("error", "Error while processing event. There is no filter with this ID...");
        sendAndClose(outputNode, socket);
        return false;
    }

    Event e(event, std::chrono::system_clock::now(), socket);
    filter->pushEvent(e);

    return true;
}

bool Controller::processInternalEvent(Jzon::Object event, int socket) 
{
    Jzon::Object outputNode;

    if (!event.Has("action") || !event.Has("params")) {
        outputNode.Add("error", "Error while processing event. Invalid JSON format...");
        sendAndClose(outputNode, socket);
        return false;
    }

    std::string action = event.Get("action").ToString();
    Jzon::Object params = event.Get("params");

    if (eventMap.count(action) <= 0) {
        outputNode.Add("error", "Error while processing event. The action does not exist...");
        sendAndClose(outputNode, socket);
        return false;
    }

    eventMap[action](&params, outputNode);
    sendAndClose(outputNode, socket);

    return true;
}

void Controller::initializeEventMap()
{
    eventMap["getState"] = std::bind(&PipelineManager::getStateEvent, pipeMngrInstance, 
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["reconfigAudioEncoder"] = std::bind(&PipelineManager::reconfigAudioEncoderEvent, pipeMngrInstance, 
                                                    std::placeholders::_1, std::placeholders::_2);
    eventMap["createPath"] = std::bind(&PipelineManager::createPathEvent, pipeMngrInstance, 
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["createFilter"] = std::bind(&PipelineManager::createFilterEvent, pipeMngrInstance, 
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["addWorker"] = std::bind(&PipelineManager::addWorkerEvent, pipeMngrInstance, 
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["addSlavesToWorker"] = std::bind(&PipelineManager::addSlavesToWorkerEvent, pipeMngrInstance, 
                                            std::placeholders::_1, std::placeholders::_2);

}


///////////////////////////////////
//PIPELINE MANAGER IMPLEMENTATION//
///////////////////////////////////

PipelineManager::PipelineManager()
{
    pipeMngrInstance = this;
    receiverID = rand();
    transmitterID = rand();
    addFilter(receiverID, SourceManager::getInstance());
    addFilter(transmitterID, SinkManager::getInstance());
    addWorker(receiverID, new LiveMediaWorker(SourceManager::getInstance()));
    addWorker(transmitterID, new LiveMediaWorker(SinkManager::getInstance()));
}

PipelineManager* PipelineManager::getInstance()
{
    if (pipeMngrInstance != NULL) {
        return pipeMngrInstance;
    }

    return new PipelineManager();
}

void PipelineManager::destroyInstance()
{
    PipelineManager* pipeMngrInstance = PipelineManager::getInstance();
    if (pipeMngrInstance != NULL) {
        delete pipeMngrInstance;
        pipeMngrInstance = NULL;
    }
}

int PipelineManager::searchFilterIDByType(FilterType type)
{
    for (auto it : filters) {
        if (it.second.first->getType() == type) {
            return it.first;
        }
    }

    return -1;
}

bool PipelineManager::addPath(int id, Path* path)
{
    if (paths.count(id) > 0) {
        return false;
    }

    paths[id] = path;

    return true;
}

BaseFilter* PipelineManager::createFilter(FilterType type)
{
    BaseFilter* filter = NULL;

    switch (type) {
        case VIDEO_DECODER:
            filter = new VideoDecoderLibav();
            break;
        case VIDEO_ENCODER:
            filter = new VideoEncoderX264();
            break;
        case VIDEO_MIXER:
            filter = new VideoMixer();
            break;
        case AUDIO_DECODER:
            filter = new AudioDecoderLibav();
            break;
        case AUDIO_ENCODER:
            filter = new AudioEncoderLibav();
            break;
        case AUDIO_MIXER:
            filter = new AudioMixer();
            break;
    }

    return filter;
}


bool PipelineManager::addFilter(int id, BaseFilter* filter)
{
    if (filters.count(id) > 0) {
        return false;
    }

    filters[id] = std::pair<BaseFilter*, Worker*>(filter, NULL);

    return true;
}

bool PipelineManager::addWorker(int id, Worker* worker)
{
    if (filters.count(id) <= 0) {
        return false;
    }

    worker->setProcessor(filters[id].first);
    filters[id].second = worker;

    return true;
}

Path* PipelineManager::getPath(int id)
{
    if (paths.count(id) <= 0) {
        return NULL;
    }

    return paths[id];
}

BaseFilter* PipelineManager::getFilter(int id)
{
    if (filters.count(id) <= 0) {
        return NULL;
    }

    return filters[id].first;
}

Worker* PipelineManager::getWorker(int id)
{
    if (filters.count(id) <= 0) {
        return NULL;
    }
    
    return filters[id].second;
}

Path* PipelineManager::createPath(int orgFilter, int dstFilter, int orgWriter, int dstReader, std::vector<int> midFilters, bool sharedQueue)
{
    Path* path;
    BaseFilter* originFilter;
    BaseFilter* destinationFilter;
    int realOrgWriter = orgWriter;
    int realDstReader = dstReader;

    if (filters.count(orgFilter) <= 0 || filters.count(dstFilter) <= 0) {
        return NULL;
    }

    for (auto it : midFilters) {
        if (filters.count(it) <= 0) {
            return NULL;
        }
    }

    originFilter = filters[orgFilter].first;
    destinationFilter = filters[dstFilter].first;

    if (realOrgWriter < 0) {
        realOrgWriter = originFilter->generateWriterID();
    }

    if (realDstReader < 0) {
        realDstReader = destinationFilter->generateReaderID();
    }

    path = new Path(orgFilter, dstFilter, realOrgWriter, realDstReader, midFilters, sharedQueue); 


    return path;
}


bool PipelineManager::connectPath(Path* path)
{
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();
    
    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        if (filters[orgFilterId].first->connectManyToMany(filters[dstFilterId].first, 
                                                          path->getDstReaderID(), 
                                                          path->getOrgWriterID(),
                                                          path->getShared()
                                                          )) 
        {
            return true;
        } else {
            utils::errorMsg("Connecting head to tail!");
            return false;
        }
    }

    if (!filters[orgFilterId].first->connectManyToOne(filters[pathFilters.front()].first, path->getOrgWriterID(), path->getShared())) {
        utils::errorMsg("Connecting path head to first filter!");
        return false;
    }

    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]].first->connectOneToOne(filters[pathFilters[i+1]].first)) {
            utils::errorMsg("Connecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()].first->connectOneToMany(filters[dstFilterId].first, path->getDstReaderID())) {
        utils::errorMsg("Connecting path last filter to path tail!");
        return false;
    }

    //TODO: manage worker assignment better
    for (auto it : pathFilters) {
        if (filters[it].second == NULL) { 
            Worker* worker = new BestEffort(filters[it].first);
            filters[it].second = worker;
            utils::debugMsg("New worker created for filter " + std::to_string(it));
            worker->start();
            utils::debugMsg("Worker " + std::to_string(it) + " started");
        }
    }

    return true;
}

bool PipelineManager::removePath(int id)
{
    Path* path = NULL;

    path = getPath(id);

    if (!path) {
        return false;
    }

    if (!deletePath(path)) {
        return false;
    }

    paths.erase(id);

    return true;
}


bool PipelineManager::deletePath(Path* path) 
{
    std::vector<int> pathFilters = path->getFilters();
    int orgFilterID = path->getOriginFilterID();
    int dstFilterID = path->getDestinationFilterID();

    if (filters.count(orgFilterID) <= 0 || filters.count(dstFilterID) <= 0) {
        return false;
    }

    for (auto it : pathFilters) {
        if (filters.count(it) <= 0) {
            return false;
        }
    }

    if (!filters[orgFilterID].first->disconnect(filters[pathFilters.front()].first, path->getOrgWriterID(), DEFAULT_ID)) {
        utils::errorMsg("Error disconnecting path head from first filter!");
        return false;
    }


    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]].first->disconnect(filters[pathFilters[i+1]].first, DEFAULT_ID, DEFAULT_ID)) {
            utils::errorMsg("Error disconnecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()].first->disconnect(filters[dstFilterID].first, DEFAULT_ID, path->getDstReaderID())) {
        utils::errorMsg("Error disconnecting path last filter to path tail!");
        return false;
    }


    for (auto it : pathFilters) {
        filters[it].second->stop();
        delete filters[it].second;
        delete filters[it].first;
    }

    delete path;

    return true;
}

void PipelineManager::startWorkers()
{   
    for (auto it : filters) {
        if (it.second.second != NULL && 
            !it.second.second->isRunning()){
            it.second.second->start();
            utils::debugMsg("Worker " + std::to_string(it.first) + " started");
        }
    }
}

void PipelineManager::stopWorkers()
{
    for (auto it : filters) {
        if (it.second.second != NULL && 
            it.second.second->isRunning()){
            it.second.second->stop();
            utils::debugMsg("Worker " + std::to_string(it.first) + " stoped");
        }
    }
}

SourceManager* PipelineManager::getReceiver()
{
    return dynamic_cast<SourceManager*>(filters[receiverID].first);
}


SinkManager* PipelineManager::getTransmitter() 
{
    return dynamic_cast<SinkManager*>(filters[transmitterID].first);
}

void PipelineManager::getStateEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    Jzon::Array filterList;
    Jzon::Array pathList;

    for (auto it : filters) {
        Jzon::Object filter;
        filter.Add("id", it.first);
        it.second.first->getState(filter);
        filterList.Add(filter);
    }

    outputNode.Add("filters", filterList);

    for (auto it : paths) {
        Jzon::Object path;
        Jzon::Array pathFilters;
        std::vector<int> pFilters = it.second->getFilters();

        path.Add("id", it.first);
        path.Add("originFilter", it.second->getOriginFilterID());
        path.Add("destinationFilter", it.second->getDestinationFilterID());
        path.Add("originWriter", it.second->getOrgWriterID());
        path.Add("destinationReader", it.second->getDstReaderID());

        for (auto it : pFilters) {
            pathFilters.Add(it);
        }

        path.Add("filters", pathFilters);
        pathList.Add(path);
    }

    outputNode.Add("paths", pathList);
}

void PipelineManager::reconfigAudioEncoderEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int encoderID, mixerID, pathID;
    int sampleRate, channels;
    Path* path;
    ACodecType codec;
    std::string sCodec;
    SinkManager* transmitter = getTransmitter();

    if (!params->Has("encoderID") || !params->Has("codec") || !params->Has("sampleRate") || !params->Has("channels")) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    encoderID = params->Get("encoderID").ToInt();
    sampleRate = params->Get("sampleRate").ToInt();
    channels = params->Get("channels").ToInt();
    sCodec = params->Get("codec").ToString();
    codec = utils::getCodecFromString(sCodec);

    for (auto it : paths) {
        if (it.second->getFilters().front() == encoderID) {
            pathID = it.first;
            path = it.second;
        }
    }

    if (!path) {
        outputNode.Add("error", "Error reconfiguring audio encoder");
        return;
    }

    mixerID = path->getOriginFilterID();

    if (!removePath(pathID)) {
        outputNode.Add("error", "Error reconfiguring audio encoder");
        return;
    }

    path = new AudioEncoderPath(mixerID, getFilter(mixerID)->generateWriterID());
    dynamic_cast<AudioEncoderLibav*>(getFilter(path->getFilters().front()))->configure(codec, channels, sampleRate);

    path->setDestinationFilter(transmitterID, transmitter->generateReaderID());

    if (!connectPath(path)) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    int encoderPathID = rand();

    if (!addPath(encoderPathID, path)) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    outputNode.Add("error", Jzon::null);

}

void PipelineManager::createFilterEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;
    FilterType fType;
    BaseFilter* filter;

    if(!params) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type")) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    fType = utils::getFilterTypeFromString(params->Get("type").ToString());

    filter = createFilter(fType);

    if (!filter) {
        outputNode.Add("error", "Error creating filter. Specified type is not correct..");
        return;
    }

    if (!addFilter(id, filter)) {
        outputNode.Add("error", "Error registering filter. Specified ID already exists..");
        return;
    }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::createPathEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    std::vector<int> filtersIds;
    int id, orgFilterId, dstFilterId;
    int orgWriterId = -1;
    int dstReaderId = -1;
    bool sharedQueue = false;
    Path* path;

    if(!params) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("orgFilterId") || 
          !params->Has("dstFilterId") || !params->Has("orgWriterId") || 
            !params->Has("dstReaderId") || !params->Has("sharedQueue")) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

   if (!params->Has("midFiltersIds") || !params->Get("midFiltersIds").IsArray()) {
      outputNode.Add("error", "Error creating path. Invalid JSON format...");
      return;
   }
        
    Jzon::Array& jsonFiltersIds = params->Get("midFiltersIds").AsArray();
    id = params->Get("id").ToInt();
    orgFilterId = params->Get("orgFilterId").ToInt();
    dstFilterId = params->Get("dstFilterId").ToInt();
    orgWriterId = params->Get("orgWriterId").ToInt();
    dstReaderId = params->Get("dstReaderId").ToInt();
    dstReaderId = params->Get("dstReaderId").ToInt();
    sharedQueue = params->Get("sharedQueue").ToBool();

    for (Jzon::Array::iterator it = jsonFiltersIds.begin(); it != jsonFiltersIds.end(); ++it) {
        filtersIds.push_back((*it).ToInt());
    }


    path = createPath(orgFilterId, dstFilterId, orgWriterId, dstReaderId, filtersIds, sharedQueue);

    if (!path) {
        outputNode.Add("error", "Error creating path. Check introduced filter IDs...");
        return;
    }

    if (!connectPath(path)) {
        outputNode.Add("error", "Error connecting path. Better pray Jesus...");
        return;
    }

    if (!addPath(id, path)) {
        outputNode.Add("error", "Error registering path. Path ID already exists...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    int id, fps;
    std::string type;
    Worker* worker = NULL;

    if(!params) {
        outputNode.Add("error", "Error creating worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type") || !params->Has("fps")) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    type = params->Get("type").ToString();
    fps = params->Get("fps").ToInt();

    if (type.compare("bestEffort") == 0) {
        worker = new BestEffort();
    } else if (type.compare("master") == 0) {
        worker = new Master();
    } else if (type.compare("slave") == 0) {
        worker = new Slave();
    }

    if (!worker) {
        outputNode.Add("error", "Error creating worker. Check type...");
        return;
    }

    if (!addWorker(id, worker)) {
        outputNode.Add("error", "Error adding worker to filter. Check filter ID...");
        return;
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addSlavesToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    Master* master = NULL;
    std::vector<Worker*> slaves;
    int masterId;


    if(!params) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("master")) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("slaves") || !params->Get("slaves").IsArray()) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    masterId = params->Get("master").ToInt();
    Jzon::Array& jsonSlavesIds = params->Get("slaves").AsArray();

    master = dynamic_cast<Master*>(getWorker(masterId));

    if (!master) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid Master ID...");
        return;
    }

    for (Jzon::Array::iterator it = jsonSlavesIds.begin(); it != jsonSlavesIds.end(); ++it) {
        slaves.push_back(getWorker((*it).ToInt()));
    }

    for (auto it : slaves) {
        if (!it) {
            outputNode.Add("error", "Error adding slaves to worker. Invalid slave ID...");
            return;
        }

        master->addSlave(dynamic_cast<Slave*>(it));
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}


/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void sendAndClose(Jzon::Object outputNode, int socket)
{
    Jzon::Writer writer(outputNode, Jzon::NoFormat);
    writer.Write();
    std::string result = writer.GetResult();
    const char* res = result.c_str();
    (void)write(socket, res, result.size());

    if (socket >= 0){
        close(socket);
    }
}
