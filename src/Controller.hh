/*
 *  Controller.hh - Controller class for livemediastreamer framework
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

#ifndef _CONTROLLER_HH
#define _CONTROLLER_HH

#include "network/SourceManager.hh"
#include "network/SinkManager.hh"
#include "Path.hh"

#define MSG_BUFFER_MAX_LENGTH 4096

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
    BaseFilter* getFilter(int id);
    SourceManager* getReceiver();
    SinkManager* getTransmitter();

    Path* getPath(int id);
    std::map<int, Path*> getPaths() {return paths;};
    bool connectPath(Path* path);   
    bool addWorkerToPath(Path *path, Worker* worker);


private:
    PipelineManager();
    static PipelineManager* pipeMngrInstance;

    std::map<int, Path*> paths;
    std::map<int, std::pair<BaseFilter*, Worker*> > filters;
    int receiverID;
    int transmitterID;

};

class WorkerManager {
public:
    static WorkerManager* getInstance();
    static void destroyInstance();

private:
    WorkerManager();
    static WorkerManager* workMngrInstance;

};

class Controller {
public:
    static Controller* getInstance();
    static void destroyInstance();
    PipelineManager* pipelineManager();
    WorkerManager* workerManager();

    bool createSocket(int port);
    int listenSocket();
    bool readAndParse(int newSock);
    
private:
    Controller();
    int sock;
    char inBuffer[MSG_BUFFER_MAX_LENGTH];
    Jzon::Object* inputRootNode;
    Jzon::Object* outputRootNode;
    Jzon::Parser* parser;

    static Controller* ctrlInstance;
    PipelineManager* pipeMngrInstance;
    WorkerManager* workMngrInstance;

};

#endif