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
 */

#ifndef _PATH_HH
#define _PATH_HH

#include <vector>

/*! Path class determines the pipeline configuration, filters interconnections
    and data paths.
*/
class Path {
public:
    /**
    * Creates a path object
    * @param origin filter Id
    * @param destination filter Id
    * @param origin writer Id
    * @param destination reader Id
    * @param list of middle id filters
    */
    Path(int originFilterID, int destinationFilterID, int orgWriterID,
            int dstReaderID, std::vector<int> midFiltersIDs);

    /**
    * Sets destination fitler
    * @param Destination filter Id
    * @param Destination reader Id
    */
    void setDestinationFilter(int destinationFilterID, int dstReaderID);

    /**
    * Gets destination reader Id
    * @return destination reader Id
    */
    const int getDstReaderID() const {return dstReaderID;};

    /**
    * Gets origin writer Id
    * @return origin writer Id
    */
    const int getOrgWriterID() const {return orgWriterID;};

    /**
    * Gets origin filter Id
    * @return origin filter Id
    */
    const int getOriginFilterID() const {return originFilterID;};

    /**
    * Gets destination filter Id
    * @return destination filter Id
    */
    const int getDestinationFilterID() const {return destinationFilterID;};

    /**
    * Gets middle filters Ids
    * @return middle filters Ids
    */
    std::vector<int> getFilters(){return filterIDs;};
    
    /**
    * Test if the path contains the given filter ID
    * @return true if the filter is present in path, false otherwise
    */
    bool hasFilter(int fId);

protected:
    void addFilterID(int filterID);

private:
    int originFilterID;
    int destinationFilterID;
    int orgWriterID;
    int dstReaderID;
    std::vector<int> filterIDs;
};


#endif
