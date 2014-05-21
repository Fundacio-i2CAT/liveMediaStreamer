/*
 *  IOProcessorInterface.hh - Input and Ouput interfaces for frame processors
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
#ifndef _IO_PROCESSOR_INTERFACE_HH
#define _IO_PROCESSOR_INTERFACE_HH

#ifndef _PROCESSOR_INTERFACE_HH
#include "ProcessorInterface.hh"
#endif

#ifndef _WORKER_HH
#include "Worker.hh"
#endif

class MultiReaderSingleWriter : public MultiReader, public SingleWriter, public Runnable {  
    
public:
    bool processFrame();
    
protected:
    MultiReaderSingleWriter();
    
    virtual bool doProcessFrame(std::map<int, Frame *> orgFrames, Frame *dst) = 0;
    bool demandFrames();
    void supplyFrame(bool newFrame);
    
private:
    //TODO these might have to be protected virtual
    Frame *dst;
    std::map<int, Frame *> orgFrames;
    std::map<int, bool> updates;
};

class SingleReaderSingleWriter : public SingleReader, public SingleWriter, public Runnable {  
    
public:
    bool processFrame();
    
protected:
    SingleReaderSingleWriter();
    
    virtual bool doProcessFrame(Frame *org, Frame *dst) = 0;
    bool demandFrame();
    void supplyFrame(bool newFrame);
    
    
private:
    //TODO these might have to be protected virtual
    Frame *org, *dst;
};

#endif
