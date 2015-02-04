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
}

Worker::~Worker()
{
}

bool Worker::addProcessor(int id, Runnable *processor) 
{
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    processor->setId(id);
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.pop();
        
        runnables[current->getId()] = current;
    }
    
    if (runnables.count(id) > 0) {
        mtx.unlock();
        return false;
    }  

    runnables[id] = processor;
    enabled = true;
    
    for (auto it : runnables){
        processors.push(it.second);
    }

    mtx.unlock();
    return true;
}

bool Worker::removeProcessor(int id)
{
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.pop();
        runnables[current->getId()] = current;
    }
    
    if (runnables.count(id) <= 0) {
        mtx.unlock();
        return false;
    }

    runnables.erase(id);
    
    for (auto it : runnables){
        processors.push(it.second);
    }
    
    mtx.unlock();
    return true;
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

//TODO: avoid void functions
void Worker::getState(Jzon::Object &workerNode)
{
    Jzon::Array pList;
    std::map<int, Runnable*> runnables;
    Runnable* current;
    
    mtx.lock();
    
    while (!processors.empty()){
        current = processors.pop();
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
    processors.begin()->second->processFrame();
    enabled = false;
}

void LiveMediaWorker::stop()
{   
    processors.begin()->second->stop();
    if (isRunning()){
        thread.join();
    }
}

///////////////////////////////////////////////////
//                MASTER CLASS                   //
///////////////////////////////////////////////////

Master::Master() : Worker() 
{
    slaves.clear();
    type = MASTER;
    frameTime = std::chrono::microseconds(0);
}

void Master::process()
{
    uint64_t teaTime = 0;
    std::chrono::microseconds active(ACTIVE);
    Runnable* currentJob;
    
    while(run) {
        startPoint = std::chrono::system_clock::now();

        mtx.lock();        
        while (!processors.empty() && processors.top()->ready()){
            currentJob = processors.pop();
            
            //TODO: remove/rethink enable feature from filters
            currentJob->processEvent();           
            currentJob->processFrame();
            currentJob->removeFrames();
            
            processors->push(currentJob);
        }
        
        currentJob = processors.top();
        
        mtx.unlock();
        
        currentJob->sleepUntilReady();
    }
}

bool Runnable::ready()
{
    return time < std::chrono::system_clock::now();
}

void Runnable::sleepUntilReady()
{
    std::system_clock::time_point now;
    if (!ready()){
        std::this_thread::sleep_for(
            std::chrono::duration_cast<std::chrono::microseconds>(time - now));
    }
}

bool Runnable::processFrame()
{
    int64_t ret;
    
    ret = doProcessframe();
    if (ret < 0){
        return false;
    } 
    
    time = std::chrono::system_clock::now() + std::chrono::microseconds(ret);
    
    return true;
}

bool Runnable::operator()(const Runnable* lhs, const Runnable* rhs)
{
    return lhs->time < rhs->time;
}
