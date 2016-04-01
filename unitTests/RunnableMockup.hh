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

class RunnableMockup : public Runnable {
    
public:
    RunnableMockup(int frameTime_, std::vector<int> enabledJobs_, bool periodic) : Runnable(periodic){
        frameTime = frameTime_;
        enabledJobs = enabledJobs_;
        first = true;
    }
    
protected:
    std::vector<int> processFrame(int& ret) {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        size_t realProcessTime;
        std::chrono::microseconds remaining, diff;
        
        if (first){
            wallclock = now;
            first = false;
        }
        
        diff = std::chrono::duration_cast<std::chrono::microseconds> (wallclock - now);
        if (std::abs(diff.count()) > frameTime/32){
            std::cout << "Failed! reseting wallcock! " << remaining.count() << std::endl;
            wallclock = now;
        }
        
        wallclock += std::chrono::microseconds(frameTime);
        
        std::uniform_int_distribution<size_t> distribution(frameTime/2, frameTime);
        realProcessTime = distribution(generator);
        std::this_thread::sleep_for(std::chrono::microseconds(realProcessTime));
        
        remaining = (std::chrono::duration_cast<std::chrono::microseconds> (wallclock - std::chrono::high_resolution_clock::now()));
        
        if (isPeriodic()){
            ret = remaining.count();
        } else {
            ret = 0;
        }
        
        return enabledJobs;
    }
    
    bool pendingJobs(){
        return false;
    }

private:
    std::default_random_engine generator;
    int frameTime;
    std::vector<int> enabledJobs;
    std::chrono::system_clock::time_point wallclock;
    bool first;
};

#endif