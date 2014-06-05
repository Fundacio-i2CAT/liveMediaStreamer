/*
 *  SourceManager.hh - Class that handles multiple sessions dynamically.
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
#ifndef _SOURCE_MANAGER_HH
#define _SOURCE_MANAGER_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _BASIC_USAGE_ENVIRONMENT_HH
#include <BasicUsageEnvironment.hh>
#endif

#ifndef _FILTER_HH
#include "../Filter.hh"
#endif

#ifndef _HANDLERS_HH
#include "Handlers.hh"
#endif

#include <thread>
#include <map>
#include <list>
#include <functional>
#include <string>

#define ID_LENGTH 4

class Session;

// typedef struct connection {
//     unsigned char rtpPayloadFormat;
//     char const* codecName;
//     unsigned channels;
//     unsigned sampleRate;
//     unsigned short port;
//     char const* session;
//     char const* medium;
// } connection_t;

class StreamClientState {
public:
    StreamClientState(std::string id_);
    virtual ~StreamClientState();
    
    std::string getId(){return id;};

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
    
private:
    std::string id;
};

class SourceManager : public HeadFilter {
private:
    SourceManager(int writersNum = MAX_WRITERS);
    
public:
    static SourceManager* getInstance();
    static void destroyInstance();
      
    bool runManager();
    bool isRunning();
    
    void closeManager();

    bool addSession(Session* session);
    bool removeSession(std::string id);
    
    Session* getSession(std::string id);
    int getWriterID(unsigned int port);
    void setCallback(std::function<void(char const*, unsigned short)> callbackFunction);
    
    UsageEnvironment* envir() {return env;};
    
private:
    friend bool handlers::addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession);
    
    void addConnection(int wId, MediaSubsession* subsession);
    
    static void* startServer(void *args);
    FrameQueue *allocQueue(int wId);
      
    std::thread mngrTh;
    
    static SourceManager* mngrInstance;
    std::map<std::string, Session*> sessionMap;
    UsageEnvironment* env;
    uint8_t watch;
    std::function<void(char const*, unsigned short)> callback;
    
};

class Session {
public:
    static Session* createNewByURL(UsageEnvironment& env, std::string progName, std::string rtspURL, std::string id);
    static Session* createNew(UsageEnvironment& env, std::string sdp, std::string id);
    
    virtual ~Session();
    
    std::string getId() {return scs->getId();};
    MediaSubsession* getSubsessionByPort(int port);
    
    bool initiateSession();
    
protected:
    Session(std::string id);
    
    RTSPClient* client;
    StreamClientState *scs;
};

#endif
