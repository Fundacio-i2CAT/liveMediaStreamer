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
#include "Worker.hh"

Worker::Worker(Runnable *processor_): processor(processor_), run(false), enabled(false)
{ 
   /* if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 0;
    }*/
	this->processor = processor_;
    enabled = true;
}

Worker::Worker(): run(false), enabled(false)
{ 
    processor = NULL;
}

void Worker::setProcessor(Runnable *processor) 
{
    /*if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 0;
    }*/

    this->processor = processor;
    enabled = true;
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

///////////////////////////////////////////////////
//            LIVEMEDIAWORKER CLASS              //
///////////////////////////////////////////////////

LiveMediaWorker::LiveMediaWorker(Runnable *processor_) : Worker(processor_){
    enabled = false;
}

void LiveMediaWorker::process()
{
    enabled = true;
    processor->processFrame();
    enabled = false;
}

void LiveMediaWorker::stop()
{   
    processor->stop();
    if (isRunning()){
        thread.join();
    }
}

///////////////////////////////////////////////////
//              BESTEFFORT CLASS                 //
///////////////////////////////////////////////////

BestEffort::BestEffort(Runnable *processor_) : Worker(processor_){
}

BestEffort::BestEffort() : Worker(){
}

void BestEffort::process() {
	int idleCount = 0;
    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);
	while(run){
		while (enabled && processor->processFrame()){
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


///////////////////////////////////////////////////
//          CONSTANTFRAMERATE CLASS              //
///////////////////////////////////////////////////

ConstantFramerate::ConstantFramerate(Runnable *processor_, unsigned int maxFps) : Worker(processor_){
	if (maxFps != 0){
		frameTime = 1000000/maxFps;
	} else {
		frameTime = 1000000/24;
	}
}

ConstantFramerate::ConstantFramerate() : Worker() {
	frameTime = 1000000/24;
}

void ConstantFramerate::setFps(unsigned int maxFps)
{
    if (maxFps != 0){
        frameTime = 1000000/maxFps;
    } else {
        frameTime = 1000000/24;
    }
}

///////////////////////////////////////////////////
//                MASTER CLASS                   //
///////////////////////////////////////////////////

Master::Master(Runnable *processor_, unsigned int maxFps):ConstantFramerate(processor_,maxFps) {
	slaves.clear();
}

Master::Master():ConstantFramerate() {
	slaves.clear();
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
	int accumulatedTime = 0;
	int threshold = int(frameTime/10);
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point previousTime;
	int idleCount = 0;
    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);

	std::atomic<bool> sync;
	sync = true;

    while(run){
        while (enabled && processor->hasFrames()) {
			if (sync) {
				previousTime = std::chrono::system_clock::now();
				processAll();
				sync = false;				
				processor->processFrame(false);
			}
			while (!allFinished()){
			}
			enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - previousTime);
			accumulatedTime+= frameTime - enlapsedTime.count();
			if (((-1) * accumulatedTime) >  threshold) {
				accumulatedTime= 0 - threshold;
			}
			while (accumulatedTime >  threshold){
                std::this_thread::sleep_for(std::chrono::microseconds(accumulatedTime - threshold));
				accumulatedTime= threshold;
            }
			processor->removeFrames();
			sync = true;
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

bool Master::allFinished() {
	bool end = true;
	if (!slaves.empty()) {
		for (std::list<Slave*>::iterator it = slaves.begin(); it != slaves.end(); it++) {
			Slave* slave = *it;
			if (slave->getFinished() == false) {
				end = false;
				break;
			}
		}
	}
	return end;
}

void Master::processAll() {
	for (std::list<Slave*>::iterator it = slaves.begin(); it != slaves.end(); it++) {
		Slave* slave = *it;
		slave->setFalse();	
	}
}

///////////////////////////////////////////////////
//                SLAVE CLASS                    //
///////////////////////////////////////////////////

Slave::Slave(int id_, Runnable *processor_, unsigned int maxFps):ConstantFramerate(processor_,maxFps) {
	id = id_;
	finished = true;
}

Slave::Slave():ConstantFramerate() {
}

void Slave::process() {
	int accumulatedTime = 0;
	int threshold = int(frameTime/10);
    std::chrono::microseconds enlapsedTime;
    std::chrono::system_clock::time_point previousTime;

    while(run){
        while (enabled && !finished) {
			finished = true;
			previousTime = std::chrono::system_clock::now();
            if (processor->processFrame(false)) {
				enlapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - previousTime);
				accumulatedTime+= frameTime - enlapsedTime.count();
				if (((-1) * accumulatedTime) >  threshold) {
					accumulatedTime= 0 - threshold;
				}
				while (accumulatedTime >  threshold){
                	std::this_thread::sleep_for(std::chrono::microseconds(accumulatedTime - threshold));
					accumulatedTime= threshold;
				}				
            }
        }
    }
}

void Slave::setFalse() {
	finished = false;
}

