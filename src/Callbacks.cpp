/*
 *  Callbacks.cpp - Collection of callback functions
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 */

#include "Callbacks.hh"
#include "Controller.hh"
#include "Filter.hh"
#include "Types.hh"
#include "Path.hh"
#include <iostream>

namespace callbacks
{
    void connectToMixerCallback(char const* medium, unsigned short port)
    {
        Controller* ctrl = Controller::getInstance();
        Path* path = NULL;
        BaseFilter *mixer = NULL;


        if (strcmp(medium, "audio") == 0) {
            path = new AudioDecoderPath(ctrl->pipelineManager()->getReceiver(), (int)port);
            mixer = ctrl->pipelineManager()->searchFilterByType(AUDIO_MIXER);

        } else if (strcmp(medium, "video") == 0) {
            path = new VideoDecoderPath(ctrl->pipelineManager()->getReceiver(), (int)port);
            mixer = ctrl->pipelineManager()->searchFilterByType(VIDEO_MIXER);
        }

        if (!path || !mixer) {
            std::cerr << "[Callback] No path nor mixer!" << std::endl;
            return;
        }

        if (!path->connect(mixer, mixer->generateReaderID())) {
            //TODO: ERROR
            return;
        }

        if (!ctrl->pipelineManager()->addPath(port, path)) {
            //TODO: ERROR
            return;
        }

        return;
    }
    
    void connectToTransmitter(char const* medium, unsigned short port)
    {
        Controller* ctrl = Controller::getInstance();
        Path* path = NULL;
        BaseFilter *mixer = NULL;
        SinkManager* transmitter = ctrl->pipelineManager()->getTransmitter();        
        
        path = new Path(ctrl->pipelineManager()->getReceiver(), (int)port);
        
        if (!path || !transmitter) {
            std::cerr << "[Callback] No path nor transmitter!" << std::endl;
            return;
        }
        
        if (!path->connect(transmitter, transmitter->generateReaderID())) {
            //TODO: ERROR
            return;
        }
        
        if (!ctrl->pipelineManager()->addPath(port, path)) {
            //TODO: ERROR
            return;
        }
        
        return;
    }
}
