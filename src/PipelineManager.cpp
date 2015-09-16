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
#include "modules/headDemuxer/HeadDemuxerLibav.hh"
#include "modules/dasher/Dasher.hh"
#include "modules/sharedMemory/SharedMemory.hh"

#define WORKER_DELETE_SLEEPING_TIME 1000 //us

PipelineManager::PipelineManager(const unsigned thds) : threads(thds)
{
    pipeMngrInstance = this;
    pool = new WorkersPool(threads);
}

PipelineManager::~PipelineManager()
{
    stop();
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
    if (pipeMngrInstance != NULL) {
        delete pipeMngrInstance;
        pipeMngrInstance = NULL;
    }
}

bool PipelineManager::stop()
{
    if (!pool){
        utils::warningMsg("No thread pool to stop!");
        return false;
    }
    
    pool->stop();
    utils::infoMsg("All threads stopped");
    
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
    
    if (pool){
        delete pool;
    }

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

bool PipelineManager::createFilter(int id, FilterType type)
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
        case DEMUXER:
            filter = new HeadDemuxerLibav();
            break;
        case VIDEO_DECODER:
            filter = new VideoDecoderLibav();
            break;
        case VIDEO_ENCODER:
            filter = new VideoEncoderX264();
            break;
         case VIDEO_RESAMPLER:
            filter = new VideoResampler();
            break;
        case VIDEO_MIXER:
            filter = VideoMixer::createNew();
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
        case DASHER:
            filter = new Dasher();
            break;            
        //TODO include sharedMemory filter
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

    if (!pool){
        utils::warningMsg("Creating new thread pool!");
        pool = new WorkersPool(threads);
    }
    
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

bool PipelineManager::createPath(int id, int orgFilter, int dstFilter, int orgWriter, int dstReader, std::vector<int> midFilters)
{
    Path* path;
    BaseFilter* originFilter;
    BaseFilter* destinationFilter;
    int realOrgWriter = orgWriter;
    int realDstReader = dstReader;

    if (paths.count(id) > 0) {
        utils::errorMsg("[PipelineManager::createPath] Path id already exists");
        return false;
    }

    if (filters.count(orgFilter) <= 0) {
        utils::errorMsg("[PipelineManager::createPath] Origin filter does not exist");
        return false;
    }

    if (filters.count(dstFilter) <= 0) {
        utils::errorMsg("[PipelineManager::createPath] Destination filter does not exist");
        return false;
    }

    for (auto it : midFilters) {
        if (filters.count(it) <= 0 || it == orgFilter || it == dstFilter) {
            utils::errorMsg("[PipelineManager::createPath] Error creating path: invalid mid filters");
            return false;
        }
    }
        
    std::vector<int> midCopy = midFilters;
    std::sort(midCopy.begin(), midCopy.end());
    std::vector<int> unique(midCopy.begin(), std::unique(midCopy.begin(), midCopy.end()));
    if (unique.size() != midFilters.size()){
        utils::errorMsg("[PipelineManager::createPath] Error creating path: duplicated mid filters");
        return false;
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
    paths[id] = path;

    return true;
}


bool PipelineManager::connectPath(int id)
{
    if (paths.count(id) <= 0) {
        utils::errorMsg("[PipelineManager::connectPath] Path does not exist");
        return false;
    }

    Path* path = paths[id];
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
    ConnectionData cData;

    if (!filters[orgFId]->isWConnected(orgWId)){
        return false;
    }

    cData = filters[orgFId]->getWConnectionData(orgWId);
    
    if (!validCData(cData, orgFId, dstFId)){
        utils::errorMsg("cData not valid to share filters");
        return false;
    }

    return filters[cData.rFilterId]->shareReader(filters[dstFId], dstRId, cData.readerId) &&
        filters[cData.rFilterId]->groupRunnable(filters[dstFId]);
}

bool PipelineManager::validCData(ConnectionData cData, int orgFId, int dstFId)
{
    if (orgFId != cData.wFilterId || dstFId == cData.rFilterId){
        return false;
    }
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
    BaseFilter* f;

    for (auto it : filters) {
        Jzon::Object filter;
        filter.Add("id", it.first);
        it.second->getState(filter);
        filterList.Add(filter);
    }

    outputNode.Add("filters", filterList);

    for (auto it : paths) {
        size_t totalPathLostBlocs = 0;
        Jzon::Object path;
        Jzon::Array pathFilters;
        std::vector<int> pFilters = it.second->getFilters();

        path.Add("id", it.first);
        path.Add("originFilter", it.second->getOriginFilterID());
        path.Add("destinationFilter", it.second->getDestinationFilterID());
        path.Add("originWriter", it.second->getOrgWriterID());
        path.Add("destinationReader", it.second->getDstReaderID());

        f = getFilter(it.second->getDestinationFilterID());
        if (f) {
            path.Add("avgDelay", (int)f->getAvgReaderDelay(it.second->getDstReaderID()).count());
            totalPathLostBlocs += f->getLostBlocs(it.second->getDstReaderID());
            for (auto itt : pFilters) {
                f = getFilter(itt);
                if (f){
                    totalPathLostBlocs += f->getLostBlocs(DEFAULT_ID);
                }
            }
            path.Add("lostBlocs", (int)totalPathLostBlocs);
        } else {
            utils::warningMsg("[PipelineManager::getStateEvent] Path filter does not exist. Ambiguous situation! Better pray Jesus...");
        }

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

    if(!params) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type")) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }
    
    if (!params->Get("id").IsNumber()){
        outputNode.Add("error", "Error creating filter. ID must be numeric...");
        return;
    }
    
    id = params->Get("id").ToInt();
    fType = utils::getFilterTypeFromString(params->Get("type").ToString());
    
    if (! createFilter(id, fType)){
        outputNode.Add("error", "Error creating filter.");
    } else {
        outputNode.Add("error", Jzon::null);
    }

}

void PipelineManager::createPathEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    std::vector<int> filtersIds;
    int id, orgFilterId, dstFilterId;
    int orgWriterId = -1;
    int dstReaderId = -1;

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
    
    if (!createPath(id, orgFilterId, dstFilterId, orgWriterId, dstReaderId, filtersIds)) {
        outputNode.Add("error", "Error creating path. Check introduced filter IDs...");
        return;
    }

    if (!connectPath(id)) {
        outputNode.Add("error", "Error connecting path. Better pray Jesus...");
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
