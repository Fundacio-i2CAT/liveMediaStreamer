/*
 *  Filter.hh - Filter base classes
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
 *            Marc Palau <marc.palau@i2cat.net>
 *            
 */

#include <map>
#include <vector>

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#ifndef _IO_INTERFACE_HH
#include "IOInterface.hh"
#endif

#ifndef _WORKER_HH
#include "Worker.hh"
#endif

class BaseFilter : Runnable {
    
public:
    bool connect(int wId, BaseFilter *R, int rId);
    bool disconnect(int wId, BaseFilter *R, int rId);
    std::vector<int> getAvailableReaders();
    std::vector<int> getAvailableWriters();
    
protected:
    BaseFilter(int readersNum, int writersNum, bool force_ = false);
    //TODO: desctructor
    Reader* getReader(int id);
    
    virtual FrameQueue *allocQueue() = 0;
    virtual bool processFrame() = 0;

    bool demandOriginFrames();
    bool demandDestinationFrames();
    void addFrames();
    void removeFrames();
    
protected:
    std::map<int, Reader*> readers;
    std::map<int, Writer*> writers;
    std::map<int, Frame*> oFrames;
    std::map<int, Frame*> dFrames;
      
private:
    int rwNextId;
    bool force;
    
    std::map<int, bool> rUpdates;
};

class OneToOneFilter : public BaseFilter {
    
protected:
    OneToOneFilter(bool force_ = false);
    //TODO: desctructor
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
    
private:
    bool processFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
};

class OneToManyFilter : public BaseFilter {
    
protected:
    OneToManyFilter(int writersNum, bool force_ = false);
    //TODO: desctructor
    virtual bool doProcessFrame(Frame *org, std::map<int, Frame *> dstFrames) = 0;
    
private:
    bool processFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
};

class ManyToOneFilter : public BaseFilter {
    
protected:
    ManyToOneFilter(int readersNum, bool force_ = false);
    //TODO: desctructor
    virtual bool doProcessFrame(std::map<int, Frame *> dstFrames, Frame *dst) = 0;
    
private:
    bool processFrame();
    using BaseFilter::demandOriginFrames;
    using BaseFilter::demandDestinationFrames;
    using BaseFilter::addFrames;
    using BaseFilter::removeFrames;
    using BaseFilter::readers;
    using BaseFilter::writers;
    using BaseFilter::oFrames;
    using BaseFilter::dFrames;
};

