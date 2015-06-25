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
#include "Worker.hh"

#include <map>

/*! PipelineManager class is a singleton class that presents the relation
    between the data flow, control and execution layers. It has all related
    information to existing filters, paths and their interconnections.
*/

class BaseFilter;

class PipelineManager {

public:
    /**
    * Gets the PipelineManger object pointer of the instance. Creates a new
    * instance for first time or returns the same instance if it already exists.
    * @return PipelineManager instance pointer
    */
    static PipelineManager* getInstance();

    /**
    * If PipelineManager instance exists it is destroyed.
    */
    static void destroyInstance();

    /**
    * Stops and clears all pipeline workes, destoys all paths and all realated filters
    */
    bool stop();

    /**
    * Creates a path
    * @param origin filter Id
    * @param destination filter Id
    * @param origin writer Id
    * @param destination reader Id
    * @param list of middle id filters
    */
    Path* createPath(int orgFilter, int dstFilter, int orgWriter,
                     int dstReader, std::vector<int> midFilters);

    /**
    * Gets filter Id by filter type, check Types.hh
    * @param FilterType type to search
    * @return Filter Id from the FilterType specified
    */
    int searchFilterIDByType(FilterType type);

    /**
    * Adds a worker by specifing its Id and the worker object
    * @param Id of the worker to add
    * @param worker object pointer
    * @return return true if succes, otherwise returns false
    */
    bool addWorker(int id, Worker* worker);

    /**
    * Adds a path by specifing its Id and the path object
    * @param Id of the path to add
    * @param path object pointer
    * @return return true if succes, otherwise returns false
    */
    bool addPath(int id, Path* path);

    /**
    * Adds a filter by specifing its Id and the filter object
    * @param Id of the filter to add
    * @param filter object pointer
    * @return return true if succes, otherwise returns false
    */
    bool addFilter(int id, BaseFilter* filter);

    /**
    * Adds a filter to a specifi worker by specifing both Ids
    * @param Id of the worker
    * @param Id of the filter
    * @return return true if succes, otherwise returns false
    */
    bool addFilterToWorker(int workerId, int filterId);

    /**
    * Gets a filter by its Id
    * @param filter Id
    * @return filter object
    */
    BaseFilter* getFilter(int id);

    /**
    * Gets a worker by its Id
    * @param worker Id
    * @return worker object
    */
    Worker* getWorker(int id);

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
    * Manage and carries out a path connection: connectManyToMany, connectManyToOne,
    * connectOneToOne and connectOneToMany
    * @return true if success, otherwise return false
    */
    bool connectPath(Path* path);

    /**
    * Starts running all workers
    */
    void startWorkers();

    /**
    * Stops all workers running
    */
    void stopWorkers();

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
    * Sets outputNode jzon object with the results coming from add worker event
    * filled by incoming jzon object params
    */
    void addWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from remove worker event
    * filled by incoming jzon object params
    */
    void removeWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from add slave to filter
    * event filled by incoming jzon object params
    */
    void addSlavesToFilterEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with the results coming from add filter to worker
    * event filled by incoming jzon object params
    */
    void addFiltersToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);

    /**
    * Sets outputNode jzon object with results of pipeline stop event
    */
    void stopEvent(Jzon::Node* params, Jzon::Object &outputNode);

private:
    PipelineManager();
    bool removePath(int id);
    bool deletePath(Path* path);
    bool removeWorker(int id);
    BaseFilter* createFilter(FilterType type, Jzon::Node* params);

    static PipelineManager* pipeMngrInstance;

    std::map<int, Path*> paths;
    std::map<int, BaseFilter*> filters;
    std::map<int, Worker*> workers;
};

#endif
