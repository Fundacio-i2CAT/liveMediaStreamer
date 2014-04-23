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

#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include "ExtendedMediaSession.hh"


//ExtendedMediaSession implementation

ExtendedMediaSession::ExtendedMediaSession(UsageEnvironment& env): MediaSession(env){};


ExtendedMediaSession* ExtendedMediaSession::createNew(UsageEnvironment& env) 
{
    ExtendedMediaSession* newSession = new ExtendedMediaSession(env);

    return newSession;
}

ExtendedMediaSession* ExtendedMediaSession::createNew(UsageEnvironment& env,
                      char const* sdpDescription) {
  ExtendedMediaSession* newSession = new ExtendedMediaSession(env);
  if (newSession != NULL) {
    if (!newSession->initializeWithSDP(sdpDescription)) {
      delete newSession;
      return NULL;
    }
  }

  return newSession;
}

void ExtendedMediaSession::setMediaSession(char* mediaSessionType, 
                                      char* sessionName, 
                                      char* sessionDescription)
{
    fMediaSessionType = mediaSessionType;
    fSessionName = sessionName;
    fSessionDescription = sessionDescription;
}
                            
Boolean ExtendedMediaSession::addSubsession(MediaSubsession* subsession)
{
    ExtendedMediaSubsession* extendedSubsession;
    int useSpecialRTPoffset = 0; //TODO: WTF is this
    
    if (subsession == NULL) {
        envir().setResultMsg("NULL MediaSubsession");
        return False;
    }
    
    if (fSubsessionsTail != NULL &&
        (extendedSubsession = dynamic_cast<ExtendedMediaSubsession*>(fSubsessionsTail)) == NULL){
        envir().setResultMsg("Failed ExtendedMediaSubsession casting");
        return False;
    }
    
    if (fSubsessionsTail == NULL) {
        fSubsessionsHead = fSubsessionsTail = subsession;
    } else {
       extendedSubsession->setNextExtendedMediaSubsession(subsession);
       fSubsessionsTail = subsession;
    }
      
    return True;
}


Boolean ExtendedMediaSession::removeSubsession(char const* subsessionName)
{
    //TODO: TBC
}

//ExtendedMediaSubsession implementation

ExtendedMediaSubsession::ExtendedMediaSubsession(MediaSession& parent): MediaSubsession(parent){};

ExtendedMediaSubsession* ExtendedMediaSubsession::createNew(MediaSession& parent)
{
    ExtendedMediaSubsession* extendedSubsession = new ExtendedMediaSubsession(parent);
    
    return extendedSubsession;
}

void ExtendedMediaSubsession::setMediaSubsession(char* mediumName, char* protocolName, 
                                                 unsigned char RTPPayloadFormat, char* codecName, 
                                                 unsigned bandwidth, unsigned RTPTimestampFrequency, 
                                                 unsigned short clientPortNum, char* controlPath)
{   
    fMediumName = mediumName;
    fProtocolName = protocolName;
    fRTPPayloadFormat = RTPPayloadFormat;
    fCodecName = codecName;
    fBandwidth = bandwidth;
    fRTPTimestampFrequency = RTPTimestampFrequency;
    fClientPortNum = clientPortNum;
    fControlPath = controlPath;
}

