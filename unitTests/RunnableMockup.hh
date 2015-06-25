/*
 *  RunnableMockup - A filter class mockup 
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            
 */

#ifndef _RUNNABLE_MOCKUP_HH
#define _RUNNABLE_MOCKUP_HH

#include <random>

#include "Worker.hh"

class RunnableMockup : public Runnable {
    
public:
    RunnableMockup(size_t frameTime_, size_t processTime_, std::string name_ = ""){
        name = name_;
        frameTime = frameTime_;
        processTime = processTime_;
        time = std::chrono::system_clock::now();
    }
    
    bool isEnabled(){return true;};
    void stop() {};
    
protected:
    std::vector<int> processFrame(int& ret) {
        std::vector<int> enabledJobs;
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        size_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        utils::debugMsg("[" + name + "] " + "StartTime " + std::to_string(millis));
        size_t realProcessTime;
        std::uniform_int_distribution<size_t> distribution(processTime/2, processTime + processTime/2);
        realProcessTime = distribution(generator);
        std::this_thread::sleep_for(std::chrono::milliseconds(realProcessTime));
        if (realProcessTime > frameTime){
            ret = 0;
            return enabledJobs;
        }
        
        utils::debugMsg("[" + name + "] " + "SleepTime " + std::to_string(frameTime - realProcessTime));
        
        ret = frameTime - realProcessTime;
        
        return enabledJobs;
    }

private:
    std::default_random_engine generator;
    std::string name;
    size_t frameTime;
    size_t processTime;
};

#endif