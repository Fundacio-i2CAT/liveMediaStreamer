/*
 *  Event.cpp - Event class
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

#include "Event.hh"
#include <iostream>

bool Event::operator<(const Event& e) const
{
    return timestamp > e.timestamp;
}

Event::Event(Jzon::Object rootNode, std::chrono::system_clock::time_point timestamp) 
{
    inputRootNode = new Jzon::Object(rootNode);
    this->timestamp = timestamp;
}

Event::~Event()
{
    //if (inputRootNode) {
    //    delete inputRootNode;
    //}
}

bool Event::canBeExecuted(std::chrono::system_clock::time_point currentTime)
{
    return currentTime > timestamp;
}

std::string Event::getAction()
{
    std::string action;

    if (inputRootNode->Has("action")) {
        action = inputRootNode->Get("action").ToString();
    }

    return action;
}

Jzon::Node* Event::getParams()
{
    if (inputRootNode->Has("params")) {
        return &inputRootNode->Get("params");
    }

    return NULL;
}




