/*
 *  PipelineManager.cpp - Pipeline manager class for livemediastreamer framework
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
 *            Gerard Castillo <gerard.castillo@i2cat.net>
 */

#include "PipelineManager.hh"
#include "modules/audioEncoder/AudioEncoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"
#include "modules/audioMixer/AudioMixer.hh"
#include "modules/videoEncoder/VideoEncoderX264.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/videoMixer/VideoMixer.hh"
#include "modules/videoResampler/VideoResampler.hh"
#include "modules/liveMediaInput/SourceManager.hh"
#include "modules/liveMediaOutput/SinkManager.hh"
#include "modules/dasher/Dasher.hh"
#include "modules/sharedMemory/SharedMemory.hh"

#define WORKER_DELETE_SLEEPING_TIME 1000 //us

PipelineManager::PipelineManager()
{
    pipeMngrInstance = this;
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

bool PipelineManager::stop()
{
    for (auto it : workers) {
        it.second->stop();
        while (it.second->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(WORKER_DELETE_SLEEPING_TIME));
        }
        delete it.second;
    }

    workers.clear();

    for (auto it : paths) {
        if (!deletePath(it.second)) {
            return false;
        }
    }

    paths.clear();

    for (auto it : filters) {
        delete it.second;
    }

    filters.clear();

    return true;
}

int PipelineManager::searchFilterIDByType(FilterType type)
{
    for (auto it : filters) {
        if (it.second->getType() == type) {
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

BaseFilter* PipelineManager::createFilter(FilterType type, Jzon::Node* params)
{
    BaseFilter* filter;
    FilterRole role;
    bool sharedFrames = true;

    if (!params->Has("role")){
        return NULL;
    }
    role = utils::getRoleTypeFromString(params->Get("role").ToString());
    if (role != MASTER && role != SLAVE){
        role = MASTER;
    }

    if (params->Has("sharedFrames")){
        sharedFrames = params->Get("sharedFrames").ToBool();
    }

    //TODO: this shouldn't be here
    int key = rand();
    VCodecType codec = RAW;

    switch (type) {
        case RECEIVER:
            filter = new SourceManager();
            break;
        case TRANSMITTER:
            filter = SinkManager::createNew();
            break;
        case VIDEO_DECODER:
            filter = new VideoDecoderLibav(role, sharedFrames);
            break;
        case VIDEO_ENCODER:
            filter = new VideoEncoderX264(role, sharedFrames);
            break;
         case VIDEO_RESAMPLER:
            filter = new VideoResampler(role, sharedFrames);
            break;
        case VIDEO_MIXER:
            filter = new VideoMixer(role, sharedFrames);
            break;
        case AUDIO_DECODER:
            filter = new AudioDecoderLibav(role, sharedFrames);
            break;
        case AUDIO_ENCODER:
            filter = new AudioEncoderLibav(role, sharedFrames);
            break;
        case AUDIO_MIXER:
            filter = new AudioMixer(role, sharedFrames);
            break;
        case SHARED_MEMORY:
            if (params->Has("codec")){
                codec = utils::getVideoCodecFromString(params->Get("codec").ToString());
                key = params->Get("key").ToInt();
            }
            filter = SharedMemory::createNew(key, codec, role, sharedFrames);
            break;
        // TODO: think about extra parameters necesary to create filters like Dasher
        // case DASHER:
        //     filter = new Dasher(sharedFrames);
        //     break;
        default:
            utils::errorMsg("Unknown filter type");
            filter = NULL;
            break;
    }

    return filter;
}

bool PipelineManager::addFilter(int id, BaseFilter* filter)
{
    if (filters.count(id) > 0) {
        utils::errorMsg("Filter ID must be unique");
        return false;
    }
    
    if (id < 0){
        utils::errorMsg("Filter ID must equal or higher than zero");
        return false;
    }

    filters[id] = filter;
    filter->setId(id);

    return true;
}

BaseFilter* PipelineManager::getFilter(int id)
{
    if (filters.count(id) <= 0) {
        utils::warningMsg("Could not find fitler ID: " + std::to_string(id));
        return NULL;
    }

    return filters[id];
}

bool PipelineManager::addWorker(int id, Worker* worker)
{
    if (workers.count(id) > 0) {
        return false;
    }

    workers[id] = worker;

    return true;
}

Worker* PipelineManager::getWorker(int id)
{
    if (workers.count(id) <= 0) {
        return NULL;
    }

    return workers[id];
}

bool PipelineManager::removeWorker(int id)
{
    Worker* worker = NULL;

    worker = getWorker(id);

    if (!worker) {
        return false;
    }

    worker->stop();
    delete workers[id];
    workers.erase(id);

    return true;
}


bool PipelineManager::addFilterToWorker(int workerId, int filterId)
{
    if (filters.count(filterId) <= 0 || workers.count(workerId) <= 0) {
        return false;
    }

    filters[filterId]->setWorkerId(workerId);

    if (!workers[workerId]->addProcessor(filterId, filters[filterId])) {
        return false;
    }

    return true;
}

Path* PipelineManager::getPath(int id)
{
    if (paths.count(id) <= 0) {
        return NULL;
    }

    return paths[id];
}

Path* PipelineManager::createPath(int orgFilter, int dstFilter, int orgWriter, int dstReader, std::vector<int> midFilters)
{
    Path* path;
    BaseFilter* originFilter;
    BaseFilter* destinationFilter;
    int realOrgWriter = orgWriter;
    int realDstReader = dstReader;

    if (filters.count(orgFilter) <= 0) {
        utils::errorMsg("Origin filter does not exist");
    }

    if (filters.count(dstFilter) <= 0) {
        utils::errorMsg("Destination filter does not exist");
    }

    if (filters.count(orgFilter) <= 0 || filters.count(dstFilter) <= 0) {
        utils::errorMsg("Error creating path: origin and/or destination filter do not exist");
        return NULL;
    }

    for (auto it : midFilters) {
        if (filters.count(it) <= 0) {
            utils::errorMsg("Error creating path: one or more of the mid filters do no exist");
            return NULL;
        }
    }

    originFilter = filters[orgFilter];
    destinationFilter = filters[dstFilter];

    if (realOrgWriter < 0) {
        realOrgWriter = originFilter->generateWriterID();
    }

    if (realDstReader < 0) {
        realDstReader = destinationFilter->generateReaderID();
    }

    path = new Path(orgFilter, dstFilter, realOrgWriter, realDstReader, midFilters);

    return path;
}


bool PipelineManager::connectPath(Path* path)
{
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();

    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        if (filters[orgFilterId]->connectManyToMany(filters[dstFilterId], path->getDstReaderID(), path->getOrgWriterID())) {
            return true;
        } else {
            utils::errorMsg("Connecting head to tail!");
            return false;
        }
    }

    if (!filters[orgFilterId]->connectManyToOne(filters[pathFilters.front()], path->getOrgWriterID())) {
        utils::errorMsg("Connecting path head to first filter!");
        return false;
    }

    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]]->connectOneToOne(filters[pathFilters[i+1]])) {
            utils::errorMsg("Connecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()]->connectOneToMany(filters[dstFilterId], path->getDstReaderID())) {
        utils::errorMsg("Connecting path last filter to path tail!");

        return false;
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
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();
    Worker *worker = NULL;

    if (filters.count(orgFilterId) <= 0 || filters.count(dstFilterId) <= 0) {
        return false;
    }

    for (auto it : pathFilters) {
        if (filters.count(it) <= 0) {
            return false;
        }
    }

    if(!filters[orgFilterId]->disconnectWriter(path->getOrgWriterID())) {
        utils::errorMsg("Error disconnecting path head!");
        return false;
    }

    if(!filters[dstFilterId]->disconnectReader(path->getDstReaderID())) {
        utils::errorMsg("Error disconnecting path tail!");
        return false;
    }

    for (auto it : pathFilters) {
        worker = getWorker(filters[it]->getWorkerId());

        if (worker) {
            worker->removeProcessor(it);
        }

        delete filters[it];
        filters.erase(it);
    }

    delete path;

    return true;
}

void PipelineManager::startWorkers()
{
    for (auto it : workers) {
        if (!it.second->isRunning()) {
            utils::debugMsg("Worker " + std::to_string(it.first) + " starting");
            it.second->start();
            utils::debugMsg("Worker " + std::to_string(it.first) + " started");
        }
    }
}

void PipelineManager::stopWorkers()
{
    for (auto it : workers) {
        if (it.second->isRunning()) {
            it.second->stop();
            utils::debugMsg("Worker " + std::to_string(it.first) + " stoped");
        }
    }
}

void PipelineManager::getStateEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    Jzon::Array filterList;
    Jzon::Array pathList;
    Jzon::Array workersList;

    for (auto it : filters) {
        Jzon::Object filter;
        filter.Add("id", it.first);
        it.second->getState(filter);
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

    for (auto it : workers) {
        Jzon::Object worker;
        worker.Add("id", it.first);
        it.second->getState(worker);
        workersList.Add(worker);
    }

    outputNode.Add("workers", workersList);

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

    filter = createFilter(fType, params);

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
    Path* path;

    if(!params) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("orgFilterId") ||
          !params->Has("dstFilterId") || !params->Has("orgWriterId") ||
            !params->Has("dstReaderId")) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    orgFilterId = params->Get("orgFilterId").ToInt();
    dstFilterId = params->Get("dstFilterId").ToInt();
    orgWriterId = params->Get("orgWriterId").ToInt();
    dstReaderId = params->Get("dstReaderId").ToInt();
    
    if (!params->Has("midFiltersIds") || !params->Get("midFiltersIds").IsArray()){
        outputNode.Add("error", "Invalid midfilters array");
        return;
    }

    Jzon::Array& jsonFiltersIds = params->Get("midFiltersIds").AsArray();

    for (Jzon::Array::iterator it = jsonFiltersIds.begin(); it != jsonFiltersIds.end(); ++it) {
        filtersIds.push_back((*it).ToInt());
    }
    
    path = createPath(orgFilterId, dstFilterId, orgWriterId, dstReaderId, filtersIds);

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

void PipelineManager::removePathEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;

    if(!params) {
        outputNode.Add("error", "Error removing path. Invalid JSON format...");
        return;
    }

    if (!params->Has("id")) {
        outputNode.Add("error", "Error removing path. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();

    if (!removePath(id)) {
        outputNode.Add("error", "Error removing path. Internal error...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::removeWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;

    if(!params) {
        outputNode.Add("error", "Error removing worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("id")) {
        outputNode.Add("error", "Error removing worker. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();

    if (!removeWorker(id)) {
        outputNode.Add("error", "Error removing worker. Internal error...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}


void PipelineManager::addWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;
    std::string type;
    Worker* worker = NULL;

    if(!params) {
        outputNode.Add("error", "Error creating worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type")) {
        outputNode.Add("error", "Error creating worker. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    type = params->Get("type").ToString();

    if (type.compare("livemedia") == 0){
        worker = new LiveMediaWorker();
    } else {
        worker = new Worker();
    }

    if (!worker) {
        outputNode.Add("error", "Error creating worker. Check type...");
        return;
    }

    if (!addWorker(id, worker)) {
        outputNode.Add("error", "Error adding worker to filter. Check filter ID...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}


void PipelineManager::addSlavesToFilterEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    BaseFilter *slave, *master;
    int masterId;

    if(!params) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("master")) {
        outputNode.Add("error", "Error adding slaves to filter. Invalid JSON format...");
        return;
    }

    if (!params->Has("slaves") || !params->Get("slaves").IsArray()) {
        outputNode.Add("error", "Error adding slaves to filter. Invalid JSON format...");
        return;
    }

    masterId = params->Get("master").ToInt();
    Jzon::Array& jsonSlavesIds = params->Get("slaves").AsArray();

    master = getFilter(masterId);

    if (!master) {
        outputNode.Add("error", "Error adding slaves to filter. Invalid Master ID...");
        return;
    }

    for (Jzon::Array::iterator it = jsonSlavesIds.begin(); it != jsonSlavesIds.end(); ++it) {
       if ((slave = getFilter((*it).ToInt())) && slave->getRole() == SLAVE){
           if (!master->addSlave((*it).ToInt(), slave)){
               outputNode.Add("error", "Error, either master or slave do not have the appropriate role!");
           }
       } else {
           outputNode.Add("error", "Error adding slaves to filter. Invalid Slave...");
       }
   }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addFiltersToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int workerId;

    if(!params) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("worker")) {
        outputNode.Add("error", "Error adding filters to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("filters") || !params->Get("filters").IsArray()) {
        outputNode.Add("error", "Error adding filters to worker. Invalid JSON format...");
        return;
    }

    workerId = params->Get("worker").ToInt();
    Jzon::Array& jsonFiltersIds = params->Get("filters").AsArray();

    for (Jzon::Array::iterator it = jsonFiltersIds.begin(); it != jsonFiltersIds.end(); ++it) {
        if (!addFilterToWorker(workerId, (*it).ToInt())) {
            outputNode.Add("error", "Error adding filters to worker. Invalid internal error...");
            return;
        }
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::stopEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    if (!stop()) {
        outputNode.Add("error", "Error stopping pipe. Internal error...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}
