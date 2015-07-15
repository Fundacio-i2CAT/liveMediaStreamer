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

PipelineManager::PipelineManager(unsigned threads)
{
    pipeMngrInstance = this;
    pool = new WorkersPool(threads);
}

PipelineManager::~PipelineManager()
{
    stop();
    if (pool){
        delete pool;
    }
    pipeMngrInstance = NULL;
}

PipelineManager* PipelineManager::getInstance(unsigned threads)
{
    if (pipeMngrInstance != NULL) {
        return pipeMngrInstance;
    }

    return new PipelineManager(threads);
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
    for (auto it : filters) {
        pool->removeTask(it.first);
    }
    utils::infoMsg("No more tasks in pool");
    
    for (auto it : paths) {
        if (!deletePath(it.second)) {
            return false;
        }
    }

    paths.clear();
    utils::infoMsg("Paths deleted");

    for (auto it : filters) {
        delete it.second;
    }

    filters.clear();
    utils::infoMsg("Filters deleted");

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


bool PipelineManager::createFilter(int id, FilterType type, FilterRole role)
{
    BaseFilter* filter = NULL;
    
    if (id < 0 || filters.count(id) > 0){
        utils::errorMsg("Invalid filter ID");
        return false;
    }

    switch (type) {
        case RECEIVER:
            filter = new SourceManager();
            break;
        case TRANSMITTER:
            filter = SinkManager::createNew();
            break;
        case VIDEO_DECODER:
            filter = new VideoDecoderLibav(role);
            break;
        case VIDEO_ENCODER:
            filter = new VideoEncoderX264(role);
            break;
         case VIDEO_RESAMPLER:
            filter = new VideoResampler(role);
            break;
        case VIDEO_MIXER:
            filter = VideoMixer::createNew(role);
            break;
        case AUDIO_DECODER:
            filter = new AudioDecoderLibav(role);
            break;
        case AUDIO_ENCODER:
            filter = new AudioEncoderLibav(role);
            break;
        case AUDIO_MIXER:
            filter = new AudioMixer(role);
            break;
        //TODO include sharedMemory and Dasher filters
        default:
            utils::errorMsg("Unknown filter type");
            break;
    }
    
    if (filter){
        addFilter(id, filter);
        return true;
    }

    return false;
}

bool PipelineManager::addFilter(int id, BaseFilter* filter)
{
    Runnable* run = NULL;
    
    if (filters.count(id) > 0) {
        utils::errorMsg("Filter ID must be unique");
        return false;
    }
    
    if (id < 0){
        utils::errorMsg("Filter ID must equal or higher than zero");
        return false;
    }

    if (!(run = dynamic_cast<Runnable*>(filter)) || !run->setId(id)){
        utils::errorMsg("Could not set filter ID");
        return false;
    }
    
    filters[id] = filter;

    return pool->addTask(filter);
}

BaseFilter* PipelineManager::getFilter(int id)
{
    if (filters.count(id) <= 0) {
        utils::warningMsg("Could not find fitler ID: " + std::to_string(id));
        return NULL;
    }

    return filters[id];
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
        return NULL;
    }

    if (filters.count(dstFilter) <= 0) {
        utils::errorMsg("Destination filter does not exist");
        return NULL;
    }

    for (auto it : midFilters) {
        if (filters.count(it) <= 0 || it == orgFilter || it == dstFilter) {
            utils::errorMsg("Error creating path: invalid mid filters");
            return NULL;
        }
    }
        
    std::vector<int> midCopy = midFilters;
    std::sort(midCopy.begin(), midCopy.end());
    std::vector<int> unique(midCopy.begin(), std::unique(midCopy.begin(), midCopy.end()));
    if (unique.size() != midFilters.size()){
        utils::errorMsg("Error creating path: duplicated mid filters");
        return NULL;
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
    
    for (auto id : pathFilters){
        if (filters.count(id) == 0){
            return false;
        }
    }

    if (pathFilters.empty()) {
        if (filters[orgFilterId]->connectManyToMany(filters[dstFilterId], path->getDstReaderID(), path->getOrgWriterID()) ||
            handleGrouping(orgFilterId, dstFilterId, path->getOrgWriterID(), path->getDstReaderID())) {
            return true;
        } else {
            utils::errorMsg("Connecting head to tail!");
            return false;
        }
    }

    if (!filters[orgFilterId]->connectManyToOne(filters[pathFilters.front()], path->getOrgWriterID()) &&
        !handleGrouping(orgFilterId, pathFilters.front(), path->getOrgWriterID(), DEFAULT_ID)) {
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

bool PipelineManager::handleGrouping(int orgFId, int dstFId, int orgWId, int dstRId)
{
    struct ConnectionData cData;
    
    if (!filters[orgFId]->isWConnected(orgWId)){
        return false;
    }
    
    cData = filters[orgFId]->getWConnectionData(orgWId);
    
    if (!validCData(cData)){
        return false;
    }
    
    return filters[cData.rFilterId]->shareReader(filters[dstFId], dstRId, cData.readerId) &&
        filters[cData.rFilterId]->groupRunnable(filters[dstFId]);
}

bool PipelineManager::validCData(struct ConnectionData cData)
{
    if (filters.count(cData.wFilterId) > 0 && filters[cData.wFilterId]->isWConnected(cData.writerId) &&
        filters.count(cData.rFilterId) > 0 && filters[cData.rFilterId]->isRConnected(cData.readerId)) {
        return true;
    }
    return false;
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
        pool->removeTask(it);
        delete filters[it];
        filters.erase(it);
    }

    delete path;

    return true;
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
}

void PipelineManager::createFilterEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;
    FilterType fType;
    FilterRole role;

    if(!params) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type") || !params->Has("role")) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }
    
    if (!params->Get("id").IsNumber()){
        outputNode.Add("error", "Error creating filter. ID must be numeric...");
        return;
    }
    
    role = utils::getRoleTypeFromString(params->Get("role").ToString());
    if (role == FR_NONE){
        utils::warningMsg("Unkown role, assuming MASTER");
        role = MASTER;
    }
    
    id = params->Get("id").ToInt();
    fType = utils::getFilterTypeFromString(params->Get("type").ToString());
    
    if (! createFilter(id, fType, role)){
        outputNode.Add("error", "Error creating filter.");
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


void PipelineManager::stopEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    if (!stop()) {
        outputNode.Add("error", "Error stopping pipe. Internal error...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}
