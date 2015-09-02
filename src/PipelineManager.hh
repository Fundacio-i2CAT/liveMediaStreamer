/*
 *  PipelineManager.hh - Pipeline manager class for livemediastreamer framework
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
 *            David Cassany <david.cassany@i2cat.net>
 *            Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _PIPELINE_MANAGER_HH
#define _PIPELINE_MANAGER_HH

#include "Filter.hh"
#include "Path.hh"
#include "WorkersPool.hh"

#include <map>

/*! PipelineManager class is a singleton class that presents the relation
    between the data flow, control and execution layers. It has all related
    information to existing filters, paths and their interconnections.
*/

class PipelineManager {

public:
    /**
    * Gets the PipelineManger object pointer of the instance. Creates a new
    * instance for first time or returns the same instance if it already exists.
    * @return PipelineManager instance pointer
    */
    static PipelineManager* getInstance(unsigned threads = 0);

    /**
    * If PipelineManager instance exists it is destroyed.
    */
    static void destroyInstance();

    /**
    * Stops and clears workers pool, destroys all paths and all realated filters
    */
    bool stop();

    /**
    * Creates a path and adds it to the internal paths map
    * @param id path id (must be unique)
    * @param origin filter Id
    * @param destination filter Id
    * @param origin writer Id
    * @param destination reader Id
    * @param list of middle id filters
    * @return true if success, false if not
    */
    bool createPath(int id, int orgFilter, int dstFilter, int orgWriter,
                     int dstReader, std::vector<int> midFilters);

    /**
    * Gets filter Id by filter type, check Types.hh
    * @param FilterType type to search
    * @return Filter Id from the FilterType specified
    */
    int searchFilterIDByType(FilterType type);

    /**
    * Adds a filter by specifing its Id and the filter object
    * @param Id of the filter to add
    * @param filter object pointer
    * @return return true if succes, otherwise returns false
    */
    bool addFilter(int id, BaseFilter* filter);


    /**
    * Gets a filter by its Id
    * @param filter Id
    * @return filter object
    */
    BaseFilter* getFilter(int id);


    /**
    * Gets a path by its Id
    * @param path Id
    * @return path object
    */
    Path* getPath(int id);

    /**
    * Gets PipelineManager's paths
    * @return a map with all path objects
    */
    std::map<int, Path*> getPaths() {return paths;};

    /**
    * Gets PipelineManager's filters
    * @return a map with all filters objects
    */
    std::map<int, BaseFilter*> getFilters() {return filters;};

    /**
    * Manage and carries out a path connection: connectManyToMany, connectManyToOne,
    * connectOneToOne and connectOneToMany
    * @param id path id
    * @return true if success, otherwise return false
    */
    bool connectPath(int id);
    
    /**
     * Remove the path related to the specified id and the related filters
     * @param int ID of the path to delete
     * @return returns true if the removal succedded, false otherwise.
     */
    bool removePath(int id);

    /**
    * Sets outputNode jzon object by getting pipeline state
    */
    void getStateEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from filter event
    * filled by incoming jzon object params
    */
    void createFilterEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from path event
    * filled by incoming jzon object params
    */
    void createPathEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from remove path event
    * filled by incoming jzon object params
    */
    void removePathEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with results of pipeline stop event
    */
    void stopEvent(Jzon::Node* params, Jzon::Object &outputNode);

private:
    PipelineManager(unsigned threads = 0);
    ~PipelineManager();
    bool deletePath(Path* path);
    bool createFilter(int id, FilterType type);
    
    bool handleGrouping(int orgFId, int dstFId, int orgWId, int dstRId);
    bool validCData(ConnectionData cData, int orgFId, int dstFId);

    static PipelineManager* pipeMngrInstance;

    std::map<int, Path*> paths;
    std::map<int, BaseFilter*> filters;
    WorkersPool *pool;
};

#endif
