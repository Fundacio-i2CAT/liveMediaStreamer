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
 *            
 */

#ifndef _WORKER_HH
#define _WORKER_HH

#include <atomic>
#include <thread>

class Runnable;

class Worker {
    
public:
    Worker(Runnable *processor_, unsigned int maxFps = 0);
    Worker();
    void setProcessor(Runnable *processor, unsigned int maxFps = 0);

    
    bool start();
    bool isRunning();
    void stop();
    void enable();
    void disable();
    bool isEnabled();
    void setFps(int maxFps);
	bool getRun(){return run;};
	bool getEnabled(){return enabled;};
	Runnable* getProcessor(){return processor;};
	unsigned int getFrameTime(){return frameTime;};
    
protected:
    void process();
private:   
    Runnable *processor;
    std::thread thread;
    std::atomic<bool> run;
    std::atomic<bool> enabled;
    //TODO: owuld be good to make it atomic, but not sure if it is lock-free
    unsigned int frameTime; //microseconds
};

class Master : public Worker {
public:
	Master(Runnable *processor_, unsigned int maxFps = 0);
protected:
	void process();	
};

class Runnable {
    
public:
    virtual bool processFrame(bool removeFrame = false) = 0;
	virtual void removeFrames() = 0;
};

#endif
