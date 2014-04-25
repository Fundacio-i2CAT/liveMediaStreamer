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

#include <liveMedia/RTSPClient.hh>
#include <thread>
#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>

#define ID_LENGTH 4

class Session;

class SourceManager {
private:
    SourceManager();
    
public:
    static SourceManager* getInstance();
    static void destroyInstance();
    
    static void randomIdGenerator(char *s, const int len);
    
    Boolean runManager();
    Boolean stopManager();
    Boolean isRunning();
    
    void closeManager();

    //Manually or RTSP or SDP
    Boolean addSession(char* id, char* mediaSessionType, char* sessionName, char* sessionDescription);
    Boolean addRTSPSession(char* id, char const* progName, char const* rtspURL);
    Session* getSession(char* id);
    
    Boolean initiateAll();
    
    Boolean removeSession(char* id);
    
    UsageEnvironment* envir() { return env; }
    
private:
      
    std::thread mngrTh;
    
    static SourceManager* mngrInstance;
    HashTable* sessionList;
    UsageEnvironment* env;
    uint8_t watch;
    
};

class Session {
public:
    static Session* createNewByURL(UsageEnvironment& env, char const* progName, char const* rtspURL);
    static Session* createNew(UsageEnvironment& env, char* mediaSessionType, char* sessionName, char* sessionDescription);
    
    virtual ~Session();
    
    Boolean initiateSession();
    
    static Boolean addSubsessionSink(UsageEnvironment& env, MediaSubsession* subsession);
    
protected:
    Session();
    
public:
    RTSPClient* client;
    MediaSession* session;
    MediaSubsessionIterator* iter;
};

#endif