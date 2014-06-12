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
 */

#include "Controller.hh"
//#include "Callbacks.hh"

Controller* Controller::ctrlInstance = NULL;
PipelineManager* PipelineManager::pipeMngrInstance = NULL;
WorkerManager* WorkerManager::workMngrInstance = NULL;

Controller::Controller()
{    
    ctrlInstance = this;
    pipeMngrInstance = PipelineManager::getInstance();
    workMngrInstance = WorkerManager::getInstance();
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

WorkerManager* Controller::workerManager()
{
    return workMngrInstance;
}

///////////////////////////////////
//PIPELINE MANAGER IMPLEMENTATION//
///////////////////////////////////

PipelineManager::PipelineManager()
{
    pipeMngrInstance = this;
    receiver = SourceManager::getInstance();
    transmitter = SinkManager::getInstance();
    //receiver->setCallback(callbacks::connectToMixerCallback);
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

BaseFilter* PipelineManager::searchFilterByType(FilterType type)
{
    for (auto it : filters) {
        if (it.second.first->getType() == type) {
            return it.second.first;
        }
    }

    return NULL;
}

bool PipelineManager::addPath(int id, Path* path)
{
    if (paths.count(id) > 0) {
        return false;
    }

    paths[id] = path;

    return true;
}

bool PipelineManager::addFilter(std::string id, BaseFilter* filter)
{
    if (filters.count(id) > 0) {
        return false;
    }

    filters[id] = std::pair<BaseFilter*, Worker*>(filter, NULL);

    return true;
}

bool PipelineManager::addWorker(std::string id, Worker* worker)
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



/////////////////////////////////
//WORKER MANAGER IMPLEMENTATION//
/////////////////////////////////

WorkerManager::WorkerManager()
{
    workMngrInstance = this;
}

WorkerManager* WorkerManager::getInstance()
{
    if (workMngrInstance != NULL) {
        return workMngrInstance;
    }

    return new WorkerManager();
}

void WorkerManager::destroyInstance()
{
    WorkerManager * workMngrInstance = WorkerManager::getInstance();
    if (workMngrInstance != NULL) {
        delete workMngrInstance;
        workMngrInstance = NULL;
    }
}
