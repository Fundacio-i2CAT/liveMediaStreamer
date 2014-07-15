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

#include <chrono>
#include <iostream>
#include "Worker.hh"

Worker::Worker(): run(false), enabled(false), pendingTask(false), canExecute(false)
{
}

bool Worker::addProcessor(int id, Runnable *processor) 
{
    if (processors.count(id) > 0) {
        return false;
    }

    if (!run) {
        processors[id] = processor;
        enabled = true;
        return true;
    }

    signalTask();

    processors[id] = processor;
    enabled = true;

    commitTask();

    return true;
}

bool Worker::removeProcessor(int id)
{
    if (processors.count(id) <= 0) {
        return false;
    }

    if (!run) {
        delete processors[id];
        processors.erase(id);
        return true;
    }

    signalTask();

    delete processors[id];
    processors.erase(id);

    commitTask();

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

void Worker::checkPendingTasks()
{
    if (pendingTask) {
        canExecute = true;

        while (pendingTask) {}
        canExecute = false;
    }
}

void Worker::signalTask()
{
    pendingTask = true;
    while (!canExecute) {}
}

void Worker::commitTask()
{
    pendingTask = false;
}

void Worker::getState(Jzon::Object &workerNode)
{
    Jzon::Array pList;
    for (auto it : processors) {
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
}

bool Master::addSlave(int id, Slave *slave) 
{
    if (slaves.size() == MAX_SLAVE) {
        return false;
    }

    if (slaves.count(id) > 0) {
        return false;
    }

    slaves[id] = slave;

    return true;        
}

bool Master::removeSlave(int id) 
{   
    if (slaves.count(id) <= 0) {
        return false;
    }

    slaves.erase(id);

    return true;
}

bool Master::allFinished() 
{
    bool end = true;

    if (slaves.empty()) {
        return end;
    }

    for (auto it : slaves) {
        if (!it.second->getFinished()) {
            end = false;
            break;
        }
    }

    return end;
}

void Master::processAll() 
{
    for (auto it : slaves) {
        it.second->setFalse();
    }
}

///////////////////////////////////////////////////
//                SLAVE CLASS                    //
///////////////////////////////////////////////////

Slave::Slave() : Worker() 
{
    finished = true;
}

void Slave::setFalse() 
{
    finished = false;
}

///////////////////////////////////////////////////
//           BEST EFFORT MASTER CLASS            //
///////////////////////////////////////////////////

BestEffortMaster::BestEffortMaster() : Master() 
{
    type = BEST_EFFORT_MASTER;
}

void BestEffortMaster::process() 
{
    int idleCount = 0;
    bool idleFlag = true;
    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);

    while(run) {
        idleFlag = true;
        checkPendingTasks();

        processAll();

        for (auto it : processors) {
            it.second->processEvent();

            if (!it.second->isEnabled() || !it.second->hasFrames()) {
                continue;
            }

            if (it.second->processFrame(false)) {
                idleFlag = false;
                idleCount = 0;
            }

        }

        while (!allFinished()) {
            std::this_thread::sleep_for(active);
        }

        for (auto it : processors) {
            it.second->removeFrames();
        }

        if (idleFlag) {
            if (idleCount <= ACTIVE_TIMEOUT){
                idleCount++;
                std::this_thread::sleep_for(active);
            } else {
                std::this_thread::sleep_for(idle);
            }
        }
    }
}

///////////////////////////////////////////////////
//           BEST EFFORT SLAVE CLASS             //
///////////////////////////////////////////////////

BestEffortSlave::BestEffortSlave() : Slave()
{
    type = BEST_EFFORT_SLAVE;
}

void BestEffortSlave::process() 
{
    int idleCount = 0;
    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);

    while(run) {
        checkPendingTasks();

        if (finished) {
            if (idleCount <= ACTIVE_TIMEOUT){
                idleCount++;
                std::this_thread::sleep_for(active);
            } else {
                std::this_thread::sleep_for(idle);
            }
            continue;
        }

        for (auto it : processors) {
            it.second->processEvent();

            if (!it.second->isEnabled()) {
                continue;
            }

            it.second->processFrame(false);

        }
        
        idleCount = 0;
        finished = true;

    }
}

///////////////////////////////////////////////////
//        CONST FRAMERATE SLAVE CLASS            //
///////////////////////////////////////////////////

ConstantFramerateMaster::ConstantFramerateMaster(unsigned int maxFps) : Master()
{
     frameTime = 1000000/maxFps;
     type = C_FRAMERATE_MASTER;
}

void ConstantFramerateMaster::setFps(unsigned int maxFps)
{
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 1000000/24;
    }
}

void ConstantFramerateMaster::process()
{
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point startPoint;
    std::chrono::microseconds chronoFrameTime(frameTime);

    while(run) {

        startPoint = std::chrono::system_clock::now();

        checkPendingTasks();

        for (auto it : processors) {
            it.second->processEvent();

            if (!enabled || !it.second->hasFrames()) {
                continue;
            }

            processAll();
            it.second->processFrame(false);

            while (!allFinished()) {
                /* Maybe we can sleep a bit here */
            }

            it.second->removeFrames();
        }

        enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startPoint);

        if (enlapsedTime < chronoFrameTime) {
            std::this_thread::sleep_for(std::chrono::microseconds(chronoFrameTime - enlapsedTime));
        }
    }
}

///////////////////////////////////////////////////
//        CONST FRAMERATE SLAVE CLASS            //
///////////////////////////////////////////////////

ConstantFramerateSlave::ConstantFramerateSlave(unsigned int maxFps) : Slave()
{
    frameTime = 1000000/maxFps;
    type = C_FRAMERATE_SLAVE;
}

void ConstantFramerateSlave::setFps(unsigned int maxFps)
{
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 1000000/24;
    }
}

void ConstantFramerateSlave::process() 
{
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point startPoint;
    std::chrono::microseconds chronoFrameTime(frameTime);

    while(run) {

        startPoint = std::chrono::system_clock::now();

        checkPendingTasks();

        for (auto it : processors) {
            it.second->processEvent();

            if (!enabled || finished) {
                continue;
            }

            finished = true;
            it.second->processFrame(false);
        }

        enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startPoint);

        if (enlapsedTime < chronoFrameTime) {
            std::this_thread::sleep_for(std::chrono::microseconds(chronoFrameTime - enlapsedTime));
        }
    }
}