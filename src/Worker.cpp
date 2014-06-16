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
 *            
 */

#define ACTIVE 400
#define IDLE 100
#define ACTIVE_TIMEOUT 500

#include <chrono>
#include "Worker.hh"
#include <iostream>



Worker::Worker(Runnable *processor_, unsigned int maxFps): processor(processor_), run(false), enabled(false)
{ 
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 0;
    }

    enabled = true;
}

Worker::Worker(): run(false), enabled(false)
{ 
    processor = NULL;
}

void Worker::setProcessor(Runnable *processor, unsigned int maxFps) 
{
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 0;
    }

    this->processor = processor;
    enabled = true;
}

void Worker::process()
{
    int idleCount = 0;
    int timeout;
    int timeToSleep = 0;
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point previousTime;

    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);


    while(run){
        while (enabled && frameTime > 0) {
            previousTime = std::chrono::system_clock::now();
            if (processor->processFrame(true)) { 
                idleCount = 0;
                enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - previousTime);
                if ((timeToSleep = frameTime - enlapsedTime.count()) <= 0){
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(timeToSleep));
                }
            } else {
                if (idleCount <= ACTIVE_TIMEOUT){
                    idleCount++;
                    std::this_thread::sleep_for(active);
                } else {
                    std::this_thread::sleep_for(idle);
                }
            }
        }      
        while (enabled && processor->processFrame(true)){
            idleCount = 0;
        }
        if (idleCount <= ACTIVE_TIMEOUT){
            idleCount++;
            std::this_thread::sleep_for(active);
        } else {
            std::this_thread::sleep_for(idle);
        }
    }
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

void Worker::setFps(int maxFps)
{
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 0;
    }
}


///////////////////////////////////////////////////
//                MASTER CLASS                   //
///////////////////////////////////////////////////

Master::Master(Runnable *processor_, unsigned int maxFps):Worker(processor_,maxFps) {
}

bool Master::addSlave(Slave *slave_) {
	if (slaves.size() == MAX_SLAVE)
		return false;
	slaves.push_back(slave_);
	return true;		
}

void Master::removeSlave(int id) {
	for (std::list<Slave*>::iterator it = slaves.begin(); it != slaves.end(); it++) {
		Slave* slave = *it;
		if (slave->getId() == id) {
			slaves.remove(slave);
			break;
		}
	}
}

void Master::process() {
    int idleCount = 0;
    int timeout;
    int timeToSleep = 0;
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point previousTime;

    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);
	bool run = getRun();
	bool enabled = getEnabled();
	Runnable* processor = getProcessor();
	//printf("aca nunca entra\n");
    while(run){
		unsigned int frameTime = getFrameTime();
		//printf ("frame time %d\n", frameTime);
        while (enabled && allFinished()) {
			//printf("enabled Master\n");
            //previousTime = std::chrono::system_clock::now();
            //if (processor->processFrame(true)) {
			setAllFinishedFalse();
			processor->removeFrames();
            idleCount = 0;
               /* enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - previousTime);
                if ((timeToSleep = frameTime - enlapsedTime.count()) <= 0){
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(timeToSleep));
                }*/
        }      
       /* while (enabled && allFinished()){
			setAllFinishedFalse();
			processor->removeFrames();
            idleCount = 0;
        }*/
       /* if (idleCount <= ACTIVE_TIMEOUT){
            idleCount++;
            std::this_thread::sleep_for(active);
        } else {
            std::this_thread::sleep_for(idle);
        }*/
    }
}

bool Master::allFinished() {
	bool end = true;
	for (std::list<Slave*>::iterator it = slaves.begin(); it != slaves.end(); it++) {
		Slave* slave = *it;
		if (slave->finished() == false) {
			end = false;
			break;
		}
	}
	return end;
}

void Master::setAllFinishedFalse() {
	for (std::list<Slave*>::iterator it = slaves.begin(); it != slaves.end(); it++) {
		Slave* slave = *it;
		slave->setFinish();
	}
}

///////////////////////////////////////////////////
//                SLAVE CLASS                    //
///////////////////////////////////////////////////

Slave::Slave(int id_, Runnable *processor_, unsigned int maxFps):Worker(processor_,maxFps) {
	id = id_;
	finish = false;
}

void Slave::process() {
    int idleCount = 0;
    int timeout;
    int timeToSleep = 0;
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point previousTime;

    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);
	bool run = getRun();
	bool enabled = getEnabled();
	Runnable* processor = getProcessor();
	unsigned int frameTime = getFrameTime();
	printf("aca nunca entra!! %d %d\n", run, enabled);
    while(run){
		//printf ("frame time %d\n", frameTime);
        while (enabled && frameTime > 0) {
            previousTime = std::chrono::system_clock::now();
			printf("Antes de procesar el frame\n");
            if (processor->processFrame(false)) {
				printf("Proceso frame\n");
				finish = true;
                idleCount = 0;
                enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - previousTime);
                if ((timeToSleep = frameTime - enlapsedTime.count()) <= 0){
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(timeToSleep));
                }
            } else {
                if (idleCount <= ACTIVE_TIMEOUT){
                    idleCount++;
                    std::this_thread::sleep_for(active);
                } else {
                    std::this_thread::sleep_for(idle);
                }
            }
        }      
      /*  while (enabled && processor->processFrame(false)){
			
			finish = true;
            idleCount = 0;
        }*/
        if (idleCount <= ACTIVE_TIMEOUT){
            idleCount++;
            std::this_thread::sleep_for(active);
        } else {
            std::this_thread::sleep_for(idle);
        }
    }
}

