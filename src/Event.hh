/*
 *  Event.hh - Event class
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            
 */

#ifndef _EVENT_HH
#define _EVENT_HH

#include <string>
#include <chrono>
#include "Jzon.h"

class Event {
    
public:
    Event(Jzon::Object rootNode, std::chrono::system_clock::time_point timestamp);
    ~Event();    
    bool canBeExecuted(std::chrono::system_clock::time_point currentTime);
    std::string getAction();
    Jzon::Node* getParams();
    bool operator<(const Event& e) const;

private:
    Jzon::Object* inputRootNode;
    std::chrono::system_clock::time_point timestamp;

};

#endif
