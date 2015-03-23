/*
 *  Path.hh - Path class
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
 *			  Martin German <martin.german@i2cat.net>
 */

#ifndef _PATH_HH
#define _PATH_HH

#include <vector>

class Path {
public:
    Path(int originFilterID, int orgWriterID, bool sharedQueue = false); 
    Path(int originFilterID, int destinationFilterID, int orgWriterID, 
            int dstReaderID, std::vector<int> midFiltersIDs, bool sharedQueue = false); 
    void setDestinationFilter(int destinationFilterID, int dstReaderID);
    const int getDstReaderID() const {return dstReaderID;};
    const int getOrgWriterID() const {return orgWriterID;};
    const int getOriginFilterID() const {return originFilterID;};
    const int getDestinationFilterID() const {return destinationFilterID;};
    std::vector<int> getFilters(){return filterIDs;};
	bool getShared(){return shared;};

protected:
    void addFilterID(int filterID);

private:
	bool shared;
    int originFilterID;
    int destinationFilterID;
    int orgWriterID;
    int dstReaderID;
    std::vector<int> filterIDs;
};


#endif
