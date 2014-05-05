/*
 *  HandlersUtils.cpp - Implementation of several handlers
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

#include "Handlers.hh"
#include "ExtendedRTSPClient.hh"
#include "QueueSink.hh"
#include <sstream>
#include <algorithm>


namespace handlers 
{
    void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void streamTimerHandler(void* clientData);
    void shutdownStream(RTSPClient* rtspClient);
    
    void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) 
    {
        do {
            UsageEnvironment& env = rtspClient->envir(); 
            StreamClientState& scs = ((ExtendedRTSPClient*)rtspClient)->scs;

            if (resultCode != 0) {
                env << "Failed to get a SDP description: " << resultString << "\n";
                delete[] resultString;
                break;
            }

            char* const sdpDescription = resultString;
            env << "Got a SDP description:\n" << sdpDescription << "\n";

            scs.session = MediaSession::createNew(env, sdpDescription);
            delete[] sdpDescription; 
            if (scs.session == NULL) {
                env << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
                break;
            } else if (!scs.session->hasSubsessions()) {
                env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
                break;
            }

            scs.iter = new MediaSubsessionIterator(*scs.session);
            setupNextSubsession(rtspClient);
            return;
        } while (0);

        shutdownStream(rtspClient);
    }
    
    void subsessionAfterPlaying(void* clientData) 
    {
        MediaSubsession* subsession = (MediaSubsession*)clientData;

        Medium::close(subsession->sink);
        subsession->sink = NULL;

        MediaSession& session = subsession->parentSession();
        MediaSubsessionIterator iter(session);
        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) return; 
        }
    }
    
    void subsessionByeHandler(void* clientData) 
    {
        MediaSubsession* subsession = (MediaSubsession*)clientData;
        subsessionAfterPlaying(subsession);
    }

    void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) 
    {
        do {
            UsageEnvironment& env = rtspClient->envir(); 
            StreamClientState& scs = ((ExtendedRTSPClient*)rtspClient)->scs; 

            if (resultCode != 0) {
                env << "Failed to set up the subsession: " << resultString << "\n";
                break;
            }

            env << "Set up the subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";
            
            handlers::addSubsessionSink(env, scs.subsession);

        } while (0);
        delete[] resultString;

        setupNextSubsession(rtspClient);
    }

    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) 
    {
        Boolean success = False;

        do {
            UsageEnvironment& env = rtspClient->envir(); 
            StreamClientState& scs = ((ExtendedRTSPClient*)rtspClient)->scs; 

            if (resultCode != 0) {
                env << "Failed to start playing session: " << resultString << "\n";
                break;
            }

            if (scs.duration > 0) {
                unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
                scs.duration += delaySlop;
                unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
                scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
            }

            env << "Started playing session";
            if (scs.duration > 0) {
                env << " (for up to " << scs.duration << " seconds)";
            }
            env << "...\n";

            success = True;
        } while (0);
        delete[] resultString;

        if (!success) {
            shutdownStream(rtspClient);
        }
    }

    void setupNextSubsession(RTSPClient* rtspClient) 
    {
        UsageEnvironment& env = rtspClient->envir(); 
        StreamClientState& scs = ((ExtendedRTSPClient*)rtspClient)->scs;
    
        scs.subsession = scs.iter->next();
        if (scs.subsession != NULL) {
            if (!scs.subsession->initiate()) {
                env << "Failed to initiate the subsession: " << env.getResultMsg() << "\n";
                setupNextSubsession(rtspClient); 
            } else {
                env << "Initiated the subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

                rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, False);
            }
            return;
        }


        if (scs.session->absStartTime() != NULL) {
            rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
        } else {
            scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
            rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
        }
    }


    void streamTimerHandler(void* clientData)
    {
        ExtendedRTSPClient* rtspClient = (ExtendedRTSPClient*)clientData;
        StreamClientState& scs = rtspClient->scs; 

        scs.streamTimerTask = NULL;

        shutdownStream(rtspClient);
    }

    void shutdownStream(RTSPClient* rtspClient)
    {
        UsageEnvironment& env = rtspClient->envir(); 
        StreamClientState& scs = ((ExtendedRTSPClient*)rtspClient)->scs; 
        
        
        if (scs.session != NULL) { 
            Boolean someSubsessionsWereActive = False;
            MediaSubsessionIterator iter(*scs.session);
            MediaSubsession* subsession;
            
            while ((subsession = iter.next()) != NULL) {
                if (subsession->sink != NULL) {
                    Medium::close(subsession->sink);
                    subsession->sink = NULL;
                    
                    if (subsession->rtcpInstance() != NULL) {
                        subsession->rtcpInstance()->setByeHandler(NULL, NULL); 
                    }
                    
                    someSubsessionsWereActive = True;
                }
            }
            
            if (someSubsessionsWereActive) {
                rtspClient->sendTeardownCommand(*scs.session, NULL);
            }
        }
        
        env << "Closing the stream.\n";
        Medium::close(rtspClient);
    }
    
    std::string makeSessionSDP(std::string sessionName, std::string sessionDescription)
    {
        std::stringstream sdp;
        sdp << "v=0\n";
        sdp << "o=- 0 0 IN IP4 127.0.0.1\n";
        sdp << "s=" << sessionName << "\n";
        sdp << "i=" << sessionDescription << "\n";
        sdp << "t= 0 0\n";
        
        return sdp.str();
    }
    
    std::string makeSubsessionSDP(std::string mediumName, std::string protocolName, 
                                  unsigned int RTPPayloadFormat, 
                                  std::string codecName, unsigned int bandwidth, 
                                  unsigned int RTPTimestampFrequency, 
                                  unsigned int clientPortNum) 
    {
        std::stringstream sdp;
        sdp << "m=" << mediumName << " " << clientPortNum;
        sdp << " RTP/AVP " << RTPPayloadFormat << "\n";
        sdp << "c=IN IP4 127.0.0.1\n";
        sdp << "b=AS:" << bandwidth << "\n";
        sdp << "a=rtpmap:" << RTPPayloadFormat << " ";
        sdp << codecName << "/" << RTPTimestampFrequency << "\n";
        if (codecName.compare("H264") == 0){
            sdp << "a=fmtp:" << RTPPayloadFormat << " packetization-mode=1\n";
        }
        
        return sdp.str();
    }
    
    char randAlphaNum(){
        static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        
        return alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    std::string randomIdGenerator(unsigned int length) {
        std::string id(length,0);
        std::generate_n(id.begin(), length, randAlphaNum);
        return id;
    }
    
    bool addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession)
    {      
        subsession->sink = QueueSink::createNew(env, subsession->mediumName());
        
        if (subsession->sink == NULL){
            return false;
        }
        
        subsession->sink->startPlaying(*(subsession->readSource()),
                                       handlers::subsessionAfterPlaying, subsession);

        if (subsession->rtcpInstance() != NULL) {
            subsession->rtcpInstance()->setByeHandler(handlers::subsessionByeHandler, subsession);
        }
        
        return true;
    }
};
