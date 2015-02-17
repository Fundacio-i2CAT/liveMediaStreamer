/*
 *  SharedMemory - Sharing frames filter through shm
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:    Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

#include "../../FrameQueue.hh"
#include "../../Filter.hh"

class SharedMemory : public OneToOneFilter {

public:
    SharedMemory(FilterRole fRole_ = MASTER);
    ~SharedMemory();

    bool doProcessFrame(Frame *org, Frame *dst);
    FrameQueue* allocQueue(int wId);

private:
    void initializeEventMap();
    //void configEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void doGetState(Jzon::Object &filterNode);

};

#endif
