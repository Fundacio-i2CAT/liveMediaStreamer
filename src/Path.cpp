/*
 *  Path.cpp - Path class
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
 */

#include <iostream>
#include "Path.hh"
#include "Controller.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"
#include "modules/audioEncoder/AudioEncoderLibav.hh"
#include "modules/videoEncoder/VideoEncoderX264.hh"

Path::Path(int originFilterID, int orgWriterID, bool sharedQueue) 
{
    this->originFilterID = originFilterID;
    this->orgWriterID = orgWriterID;
    destinationFilterID = -1;
    dstReaderID = -1;
	shared = sharedQueue;
}

void Path::addFilterID(int filterID)
{
    filterIDs.push_back(filterID);
}

void Path::setDestinationFilter(int destinationFilterID, int dstReaderID)
{
    this->destinationFilterID = destinationFilterID;
    this->dstReaderID = dstReaderID;
}

VideoDecoderPath::VideoDecoderPath(int originFilterID, int orgWriterID, bool sharedQueue) : 
Path(originFilterID, orgWriterID, sharedQueue)
{
	VideoDecoderLibav *decoder = new VideoDecoderLibav();
    int id = rand();
    Controller::getInstance()->pipelineManager()->addFilter(id, decoder);
    addFilterID(id);
 //   VideoDecoderLibav *decoder = new VideoDecoderLibav();
 //   addFilter(decoder);
}

VideoEncoderPath::VideoEncoderPath(int originFilterID, int orgWriterID, bool sharedQueue) :
Path(originFilterID, orgWriterID, sharedQueue)
{
    VideoEncoderX264 *encoder = new VideoEncoderX264();
    int id = rand();
	printf("ID del encoder: %d\n", id);
    Controller::getInstance()->pipelineManager()->addFilter(id, encoder);
    addFilterID(id);
}

AudioDecoderPath::AudioDecoderPath(int originFilterID, int orgWriterID, bool sharedQueue) :
Path(originFilterID, orgWriterID, sharedQueue)
{
    AudioDecoderLibav *decoder = new AudioDecoderLibav();
    int id = rand();
    Controller::getInstance()->pipelineManager()->addFilter(id, decoder);
    addFilterID(id);
}

AudioEncoderPath::AudioEncoderPath(int originFilterID, int orgWriterID, bool sharedQueue) :
Path(originFilterID, orgWriterID, sharedQueue)
{
    AudioEncoderLibav *encoder = new AudioEncoderLibav();
    int id = rand();
    Controller::getInstance()->pipelineManager()->addFilter(id, encoder);
    addFilterID(id);
}
