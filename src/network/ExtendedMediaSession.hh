/*
 *  ExtendedMediaSession.cpp - Class that handles multiple streams dynamically
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

#ifndef _EXTENDED_MEDIA_SESSION_HH
#define _EXTENDED_MEDIA_SESSION_HH

#include <MediaSession.hh>

class ExtendedMediaSubsession;

class ExtendedMediaSession: public MediaSession {
       
public:
    
    static ExtendedMediaSession* createNew(UsageEnvironment& env);
    static ExtendedMediaSession* createNew(UsageEnvironment& env, char const* sdpDescription);
    
    //TODO: shouldn't it be const char* ?
    void setMediaSession(char* mediaSessionType, char* sessionName, char* sessionDescription);
    
    Boolean addSubsession(MediaSubsession* subsession);
    Boolean removeSubsession(char const* subsessionName); //TODO: should we use something different? ControlPath?
    
protected:
    
     ExtendedMediaSession(UsageEnvironment& env);
    
};

class ExtendedMediaSubsession: public MediaSubsession {

public:
        
    static ExtendedMediaSubsession* createNew(MediaSession& parent);
    
    void setMediaSubsession(char* mediumName, char* protocolName, 
                            unsigned char RTPPayloadFormat, 
                            char* codecName, unsigned bandwidth, 
                            unsigned RTPTimestampFrequency, 
                            unsigned short clientPortNum = 0, char* controlPath = NULL);
        
    void setNextExtendedMediaSubsession(MediaSubsession *next) { fNext = next; };
      
protected:
    ExtendedMediaSubsession(MediaSession& parent);
};

#endif
