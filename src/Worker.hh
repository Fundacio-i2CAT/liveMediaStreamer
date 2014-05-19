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

#include <thread>

#ifndef _PROCESSOR_INTERFACE_HH
#include "ProcessorInterface.hh"
#endif

class Worker {
    
public:
    Worker(ReaderWriter *processor_);
    
    bool start();
    bool isRunning();
    void stop();
    void enable();
    void disable();
    bool isEnabled();
    
private:
    void process();
    
    ReaderWriter *processor;
    std::thread thread;
    std::atomic<bool> run;
    std::atomic<bool> enabled;
};

#endif
