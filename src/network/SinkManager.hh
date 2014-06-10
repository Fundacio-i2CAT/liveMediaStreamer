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

#ifndef _BASIC_USAGE_ENVIRONMENT_HH
#include <BasicUsageEnvironment.hh>
#endif

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _QUEUE_SOURCE_HH
#include "QueueSource.hh"
#endif

#ifndef _FILTER_HH
#include "../Filter.hh"
#endif

#include <thread>
#include <map>
#include <string>

#define RTSP_PORT 8554

class SinkManager : public TailFilter {
private:
    SinkManager(int readersNum = MAX_READERS);
    
public:
    static SinkManager* getInstance();
    static void destroyInstance();

    bool runManager();
    bool isRunning();
    
    void closeManager();

    bool addSession(std::string id, std::vector<int> readers, 
                    std::string info = NULL, std::string desc = NULL);
    
    ServerMediaSession* getSession(std::string id); 
    bool publishSession(std::string id);
    bool removeSession(std::string id);
    
    UsageEnvironment* envir() {return env;}
    
protected:
    Reader *setReader(int readerID, FrameQueue* queue);
    
private: 
    ServerMediaSubsession *createSubsessionByReader(Reader *reader);
    ServerMediaSubsession *createVideoMediaSubsession(VCodecType codec, QueueSource *source);
    
    std::thread mngrTh;
   
    static SinkManager* mngrInstance;
    std::map<std::string, ServerMediaSession*> sessionList;
    UsageEnvironment* env;
    uint8_t watch;
    
    RTSPServer* rtspServer;
};

#endif
