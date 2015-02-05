/*
 *  Worker.cpp - Worker class implementation
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
 *  		  Martin German <martin.german@i2cat.net>
 *            
 */

#define ACTIVE 400
#define IDLE 100
#define ACTIVE_TIMEOUT 500

#include <cmath>
#include <iostream>
#include <algorithm>
#include "Worker.hh"

Worker::Worker(): run(false), enabled(false)
{
    type = MASTER;
}

Worker::~Worker()
{
}

bool Worker::addProcessor(int id, Runnable *processor) 
{
    bool ret = false;
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.top();
        processors.pop();
        runnables[current->getId()] = current;
    }
    
    if (runnables.count(id) == 0) {
        processor->setId(id);
        runnables[id] = processor;
        enabled = true; 
        ret = true;
        
    } 
    
    for (auto it : runnables){
        processors.push(it.second);
    }
    
    mtx.unlock();
    return ret;
}

bool Worker::removeProcessor(int id)
{
    bool ret = false;
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.top();
        processors.pop();
        runnables[current->getId()] = current;
    }
    
    while (runnables.count(id) > 0) {
        runnables.erase(id);
        ret = true;
    }
    
    for (auto it : runnables){
        processors.push(it.second);
    }
    
    mtx.unlock();
    return ret;
}

bool Worker::start()
{
    run = true;
    thread = std::thread(&Worker::process, this);
    return thread.joinable();
}

bool Worker::isRunning()
{
    return thread.joinable();
}

void Worker::stop()
{
    if (isRunning()){
        run = false;
        if (isRunning()){
            thread.join();
        }
    }
}

void Worker::enable()
{
    enabled = true;
}

void Worker::disable()
{
    enabled = false;
}

bool Worker::isEnabled()
{
    return enabled;
}

void Worker::process()
{
    Runnable* currentJob;
    
    while(run) {
        mtx.lock();        
        
        while (!processors.empty() && processors.top()->ready()){
            currentJob = processors.top();
            processors.pop();
            //TODO: remove/rethink enable feature from filters
            currentJob->processEvent(); 
            currentJob->runProcessFrame();
            currentJob->removeFrames();
            processors.push(currentJob);
        }
        
        currentJob = processors.top();
        
        mtx.unlock();
        
        currentJob->sleepUntilReady();
    }
}

//TODO: avoid void functions
void Worker::getState(Jzon::Object &workerNode)
{
    Jzon::Array pList;
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.top();
        processors.pop();
        runnables[current->getId()] = current;
    }
    
    for (auto it : runnables){
        processors.push(it.second);
    }
    
    mtx.unlock();
    
    for (auto it : runnables) {
        pList.Add(it.first);
    }

    workerNode.Add("type", utils::getWorkerTypeAsString(type));
    workerNode.Add("processors", pList);
}



///////////////////////////////////////////////////
//            LIVEMEDIAWORKER CLASS              //
///////////////////////////////////////////////////

LiveMediaWorker::LiveMediaWorker() : Worker()
{
    enabled = false;
    type = LIVEMEDIA;
}

void LiveMediaWorker::process()
{
    enabled = true;
    processors.top()->runProcessFrame();
    enabled = false;
}

void LiveMediaWorker::stop()
{   
    processors.top()->stop();
    if (isRunning()){
        thread.join();
    }
}

bool Runnable::ready()
{
    return time < std::chrono::system_clock::now();
}

void Runnable::sleepUntilReady()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::microseconds teaTime;
    
    if (!ready()){
        teaTime = std::chrono::duration_cast<std::chrono::microseconds>(time - now);
        
        std::this_thread::sleep_for(teaTime);
    }
}

bool Runnable::runProcessFrame()
{
    size_t ret;
    
    ret = processFrame();
    if (ret < 0){
        return false;
    } 
    
    time = std::chrono::system_clock::now() + std::chrono::microseconds(ret);
       
    return true;
}
