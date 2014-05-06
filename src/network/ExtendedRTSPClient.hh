/*
 *  ExtendedRTSPClient.hh - Class that handles an RTSP session
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



#ifndef _EXTENDED_RTSP_CLIENT_HH
#define _EXTENDED_RTSP_CLIENT_HH


#ifndef _LIVEMEDIA_HH
#include <liveMedia.hh>
#endif

#include <BasicUsageEnvironment.hh>

class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
};

class ExtendedRTSPClient: public RTSPClient {
public:
    static ExtendedRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
                  int verbosityLevel = 0,
                  char const* applicationName = NULL,
                  portNumBits tunnelOverHTTPPortNum = 0);
    

protected:
    ExtendedRTSPClient(UsageEnvironment& env, char const* rtspURL,
        int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);

    virtual ~ExtendedRTSPClient();

public:
    StreamClientState scs;
};



#endif