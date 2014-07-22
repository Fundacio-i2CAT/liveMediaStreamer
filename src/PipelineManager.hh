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
 */

#ifndef _PIPELINE_MANAGER_HH
#define _PIPELINE_MANAGER_HH

#include "modules/audioEncoder/AudioEncoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"
#include "modules/audioMixer/AudioMixer.hh"
#include "modules/videoEncoder/VideoEncoderX264.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/videoMixer/VideoMixer.hh"
#include "modules/videoResampler/VideoResampler.hh"
#include "modules/liveMediaInput/SourceManager.hh"
#include "modules/liveMediaOutput/SinkManager.hh"

#include "Path.hh"

#include <map>

class PipelineManager {

public:
    static PipelineManager* getInstance();
    static void destroyInstance();
    int getReceiverID() {return receiverID;};
    int getTransmitterID() {return transmitterID;};
    int searchFilterIDByType(FilterType type);
    bool addPath(int id, Path* path);
    bool addWorker(int id, Worker* worker);
    bool addFilter(int id, BaseFilter* filter);
    bool addFilterToWorker(int workerId, int filterId);

    BaseFilter* getFilter(int id);
    Worker* getWorker(int id);
    SourceManager* getReceiver();
    SinkManager* getTransmitter();

    Path* getPath(int id);
    std::map<int, Path*> getPaths() {return paths;};
    bool connectPath(Path* path);   
    bool addWorkerToPath(Path *path, Worker* worker = NULL);
    void startWorkers();
    void stopWorkers();
    void getStateEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void reconfigAudioEncoderEvent(Jzon::Node* params, Jzon::Object &outpuNode);
    void createFilterEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void createPathEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void addWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void addSlavesToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void addFiltersToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode);


private:
    PipelineManager();
    bool removePath(int id);
    bool deletePath(Path* path); 
    BaseFilter* createFilter(FilterType type);
    Path* createPath(int orgFilter, int dstFilter, int orgWriter, int dstReader, std::vector<int> midFilters, bool sharedQueue = false);
    
    static PipelineManager* pipeMngrInstance;

    std::map<int, Path*> paths;
    std::map<int, BaseFilter*> filters;
    std::map<int, Worker*> workers;
    int receiverID;
    int transmitterID;
};

#endif