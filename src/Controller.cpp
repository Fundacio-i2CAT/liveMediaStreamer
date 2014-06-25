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

bool Controller::processEvent()
{
    if (inputRootNode->Get("filterID").ToInt() != 0) {
        return processFilterEvent();
    }
    
    return processInternalEvent();
}

bool Controller::processFilterEvent() 
{
    int filterID = -1;
    BaseFilter *filter = NULL;

    if (!inputRootNode->Has("action") || !inputRootNode->Has("params")) {
        //TODO: error
        return false;
    }

    filterID = inputRootNode->Get("filterID").ToInt();
    filter = pipeMngrInstance->getFilter(filterID);

    if (!filter) {
        //TODO: error
        return false;
    }

    Event e(*inputRootNode, std::chrono::system_clock::now(), connectionSocket);
    filter->pushEvent(e);

    return true;
}

bool Controller::processInternalEvent() 
{
    Jzon::Object outputNode;

    if (!inputRootNode->Has("action") || !inputRootNode->Has("params")) {
        //TODO: error
        return false;
    }

    std::string action = inputRootNode->Get("action").ToString();
    Jzon::Object params = inputRootNode->Get("params");

    if (eventMap.count(action) <= 0) {
        return false;
    }
        
    eventMap[action](&params, outputNode);
    sendAndClose(outputNode, connectionSocket);

    return true;
}

void Controller::initializeEventMap()
{
    eventMap["getState"] = std::bind(&PipelineManager::getStateEvent, pipeMngrInstance, 
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

bool PipelineManager::connectPath(Path* path)
{
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();
    
    if (filters.count(orgFilterId) <= 0) {
        return false;
    }

    if (filters.count(dstFilterId) <= 0) {
        return false;
    }

    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        if (filters[orgFilterId].first->connectManyToMany(filters[dstFilterId].first, path->getDstReaderID(), path->getOrgWriterID())) {
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
        if (filters[it].second == NULL){
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
        utils::errorMsg("Error disconnectinc path head from first filter!");
        return false;
    }

    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]].first->disconnect(filters[pathFilters[i+1]].first, DEFAULT_ID, DEFAULT_ID)) {
            utils::errorMsg("Connecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()].first->disconnect(filters[dstFilterID].first, DEFAULT_ID, path->getDstReaderID())) {
        utils::errorMsg("Connecting path last filter to path tail!");
        return false;
    }

    for (auto it : pathFilters) {
        filters[it].second->stop();
        delete filters[it].second;
        delete filters[it].first;
    }

    delete path;
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
