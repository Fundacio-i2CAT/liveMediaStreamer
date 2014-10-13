/*
 *  Worker.cpp - Is the owner of a Processor thread
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

#ifndef _WORKER_HH
#define _WORKER_HH

#include <atomic>
#include <thread>
#include <map>
#include <chrono>
#include "Frame.hh"
#include "Types.hh"
#include "Jzon.h"
#include "Utils.hh"
#include <mutex>

#define MAX_SLAVE 16

class Runnable;

class Worker {
    
public:
    Worker(Runnable *processor_);
    Worker();
    virtual ~Worker();
    
    bool start();
    bool isRunning();
    virtual void stop();
    virtual void enable();
    virtual void disable();
    bool isEnabled();
    bool addProcessor(int id, Runnable *processor);
    bool removeProcessor(int id);
    WorkerType getType(){return type;};
    std::map<int, Runnable*> getProcessors(){return processors;};
    void getState(Jzon::Object &workerNode);
    
protected:
    virtual void process() = 0;

    std::map<int, Runnable*> processors;
    std::thread thread;
    std::atomic<bool> run;
    std::atomic<bool> enabled;

    WorkerType type;
    std::mutex mtx;
};

class LiveMediaWorker : public Worker {
public:
    LiveMediaWorker();
    void enable() {};
    void disable() {};
    void stop();
private:
    void process();
};

class Slave : public Worker {
public:
    Slave();
    bool getFinished(){return finished;};
    void setFalse();
    
protected:
    void process();
    std::atomic<bool> finished; 
};

class Master : public Worker {
public:
    Master();
    bool addSlave(int id, Slave *slave);
    bool removeSlave(int id);

protected:
    void process();
    bool allFinished();
    void processAll();

private:
    void updateFrameTime(Runnable* processor);
    std::map<int, Slave*> slaves;
    std::chrono::microseconds frameTime;

};

class Runnable {
    
public:
    ~Runnable(){};
    virtual bool processFrame(bool removeFrame = true) = 0;
    virtual void processEvent() = 0;
    virtual void removeFrames() = 0;
    virtual bool hasFrames() = 0;
    virtual bool isEnabled() = 0;
    virtual void stop() = 0;
    virtual std::chrono::microseconds getFrameTime() = 0;
};

#endif
