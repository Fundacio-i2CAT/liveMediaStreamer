/*
 *  Runnable.hh - This is the interface between Workers and Filters
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *
 *
 */

#ifndef _RUNNABLE_HH
#define _RUNNABLE_HH

#include <chrono>

#include "Utils.hh"

class Runnable {
    
public:
    virtual ~Runnable(){};
    
    /**
    * This method runs the pure virtual method processFrame and sets the next time value
    * when processFrame should run 
    */
    bool runProcessFrame();
    virtual bool isEnabled() = 0;
    
    /**
    * This method stop the filter if, and just if, this is a liveMediaFilter
    */
    virtual void stop() = 0;
    
    /**
    * This method tests if enough time went through since last processFrame
    * @return True if last processFrame execution started more than one frame time ago
    */
    bool ready();
    
    /**
    * This method sleeps until processFrame is executable
    */
    void sleepUntilReady();
    
    /**
    * @return Id of the runnable object
    */
    int getId() {return id;};
    
    /**
    * sets the Runnable object id
    * @param Id of the runnable object
    */
    void setId(int id_) {id = id_;};
    //TODO: setId should be private
    
    /**
    * Operator definition to make Runnable objects comparable
    * @param Id of the runnable object
    */
    bool operator()(const Runnable* lhs, const Runnable* rhs);
    
    /**
    * Operator definition to make Runnable objects comparable
    * @return time point of the next execution of processFrame
    */
    std::chrono::system_clock::time_point getTime() const {return time;};
    
protected:
    virtual size_t processFrame() = 0;
    std::chrono::system_clock::time_point time;
    
private:
    int id;
};

struct RunnableLess : public std::binary_function<Runnable*, Runnable*, bool>                                                                                     
{
  bool operator()(const Runnable* lhs, const Runnable* rhs) const
  {
    return lhs->getTime() > rhs->getTime();
  }
};

#endif
