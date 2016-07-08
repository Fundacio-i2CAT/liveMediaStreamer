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
 *            David Cassany <david.cassany@i2cat.net>
 */

#include <algorithm>

#include "Path.hh"

Path::Path(int originFilterID, int destinationFilterID, int orgWriterID, 
            int dstReaderID, std::vector<int> midFiltersIDs)
{
    this->originFilterID = originFilterID;
    this->orgWriterID = orgWriterID;
    this->destinationFilterID = destinationFilterID;
    this->dstReaderID = dstReaderID;

    filterIDs = midFiltersIDs;
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

bool Path::hasFilter(int fId)
{
    if (std::find(filterIDs.begin(), filterIDs.end(), fId) != filterIDs.end()){
        return true;
    }

    
    if (fId == originFilterID || fId == destinationFilterID){
        return true;
    }
    
    return false;
}
