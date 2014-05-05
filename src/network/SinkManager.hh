/*
 *  SinkManager.hh - Class that handles multiple server sessions dynamically.
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *            
 */
#ifndef _SINK_MANAGER_HH
#define _SINK_MANAGER_HH

#define RTSP_PORT 8554

#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include <thread>


#define ID_LENGTH 4


class SinkManager {
private:
    SinkManager();
    
public:
    static SinkManager* getInstance();
    static void destroyInstance();
    
    //TODO: common with source manager, shall we add it in handlers namespace?
    static void randomIdGenerator(char *s, const int len);
    
    Boolean runManager();
    Boolean stopManager();
    Boolean isRunning();
    
    void closeManager();

    Boolean addSession(char* id, char const* streamName = NULL,
                       char const* info = NULL,
                       char const* description = NULL);
    
    ServerMediaSession* getSession(char* id); 
    Boolean publishSession(char* id);
    Boolean removeSession(char* id);
    
    UsageEnvironment* envir() { return env; }
    
private:
      
    std::thread mngrTh;
    
    static SinkManager* mngrInstance;
    HashTable* sessionList;
    UsageEnvironment* env;
    uint8_t watch;
    
    RTSPServer* rtspServer;
    
};



#endif