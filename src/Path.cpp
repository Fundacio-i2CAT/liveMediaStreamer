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
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"
#include "modules/audioEncoder/AudioEncoderLibav.hh"

Path::Path(BaseFilter *origin, int orgWriterID) 
{
    this->origin = origin;
    this->orgWriterID = orgWriterID;
    destination = NULL;
    dstReaderID = -1;

    if(!origin->connectManyToOne(filters.front(), orgWriterID)) {
        std::cerr << "Error!" << std::endl;
    }

    for (unsigned i = 0; i < filters.size() - 1; i++) {
        if(!filters[i]->connectOneToOne(filters[i+1])) {
            std::cerr << "Error!" << std::endl;
        }
    }
}

void Path::addFilter(BaseFilter *filter)
{
    filters.push_back(filter);
}


bool Path::connect(BaseFilter *destination, int dstReaderID)
{
    this->destination = destination;
    this->dstReaderID = dstReaderID;

    if(!filters.back()->connectOneToMany(destination, dstReaderID)) {
        std::cerr << "Error!" << std::endl;
        return false;
    }

    return true;
}

VideoDecoderPath::VideoDecoderPath(BaseFilter *origin, int orgWriterID) : 
Path(origin, orgWriterID)
{
    VideoDecoderLibav *decoder = new VideoDecoderLibav();
    addFilter(decoder);
}

AudioDecoderPath::AudioDecoderPath(BaseFilter *origin, int orgWriterID) :
Path(origin, orgWriterID)
{
    AudioDecoderLibav *decoder = new AudioDecoderLibav();
    addFilter(decoder);
}

AudioEncoderPath::AudioEncoderPath(BaseFilter *origin, int orgWriterID) :
Path(origin, orgWriterID)
{
    AudioEncoderLibav *encoder = new AudioEncoderLibav();
    addFilter(encoder);
}
