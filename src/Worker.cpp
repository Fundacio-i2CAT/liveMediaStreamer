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
#define IDLE 20
#define TIMEOUT 400

#include <chrono>
#include "Worker.hh"



Worker::Worker(ReaderWriter *processor_): processor(processor_), run(false), enabled(false)
{ 
}

void Worker::process()
{
    int idleCount = 0;
    std::chrono::microseconds active(ACTIVE);
    std::chrono::milliseconds idle(IDLE);
    
    while(run){
        while(enabled && processor->processFrame()){
            idleCount = 0;
        }
        if (idleCount == TIMEOUT){
            idleCount++;
            std::this_thread::sleep_for(active);
        } else {
            std::this_thread::sleep_for(idle);
        }
    }
}

bool Worker::start()
{
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
