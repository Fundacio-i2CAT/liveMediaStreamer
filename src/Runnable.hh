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
#include <functional>
#include <vector>

#include "Utils.hh"


/*! Runnable class is an interface implemented by BaseFilter, which has some
    basic methods in order to process a single frame of the filter.
*/
class Runnable {

public:
    virtual ~Runnable(){};

    std::vector<int> runProcessFrame();

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
     * Gets the runnable Id, if Id < 0 it means unset ID, only zero or higher values are allowed
     * @return id of the filter, it is a unique value.
     */
    int getId() {return id;};

    /**
    * Operator definition to make Runnable objects comparable
    * @param pointer to left side runnable object
    * @param pointer to right side runnable object
    */
    bool operator()(const Runnable* lhs, const Runnable* rhs);
    
    /**
     * Returns the value of the running flag
     * @return True if the runnable is currently running, False otherwise
     */
    bool isRunning() {return running;};
    
    /**
     * Sets the running flag to true
     */
    void setRunning() {running = true;};
    
    /**
     * Sets the running flag to false
     */
    void unsetRunning() {running = false;};

    /**
    * Get next time point of processFrame execution
    * @return time point of the next execution of processFrame
    */
    std::chrono::system_clock::time_point getTime() const {return time;};
    
    /**
     * This method test if the runnable is periodic or not
     * @return true is the Runnable is periodic, false otherwise
     */
    bool isPeriodic() const {return periodic;};
    
    //TODO should be a private method
    void setId(int id_);
    

protected:
    /**
     * Runnable constructor
     * @param bool it determines if the Runnable is periodic or not
     */
    Runnable(bool periodic = false);
    
    /**
     * This is the virtual method that derivatives classes implements to process data
     * @param integer this integer contains the delay until the method can be executed again
     * @return A vector containing the ids of the runnables that can be exectued after 
     * this process (e.g new data has been generated)
     */
    virtual std::vector<int> processFrame(int& ret) = 0;
    
protected:
    std::chrono::system_clock::time_point time;

private:
    const bool periodic;
    bool running;
    int id;
};


struct RunnableLess : public std::binary_function<Runnable*, Runnable*, bool>
{
  bool operator()(const Runnable* lhs, const Runnable* rhs) const
  {
    return lhs->getTime() < rhs->getTime();
  }
};


#endif
