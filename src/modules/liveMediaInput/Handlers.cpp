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
#include "QueueSink.hh"
#include "H264VideoSdpParser.hh"
#include "SourceManager.hh"
#include "ExtendedRTSPClient.hh"
#include "../../AVFramedQueue.hh"

#include <iostream>
#include <sstream>
#include <algorithm>

namespace handlers
{
    void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void getOptions(RTSPClient* rtspClient, RTSPClient::responseHandler* afterFunc);
    void getParameterCommand(RTSPClient* rtspClient, RTSPClient::responseHandler* afterFunc);
    void shutdownStream(RTSPClient* rtspClient);
    std::string modifySessionName(std::string sdp, std::string sessionName);
    void streamTimerHandler(void* clientData);
    void checkSessionTimeoutBrokenServer(void* clientData);

    void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
    {
        do {
            UsageEnvironment& env = rtspClient->envir();
            StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

            if (resultCode != 0) {
                env << "Failed to get a SDP description: " << resultString << "\n";
                delete[] resultString;
                break;
            }

            char* const sdpDescription = resultString;
            env << "Got a SDP description:\n" << sdpDescription << "\n";

            scs.session = MediaSession::createNew(env, modifySessionName(std::string(sdpDescription), scs.getId()).c_str());
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
        QueueSink *queueSink;

        do {
            UsageEnvironment& env = rtspClient->envir();
            StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

            if (resultCode != 0) {
                env << "Failed to set up the subsession: " << resultString << "\n";
                break;
            }

            env << "Set up the subsession (client ports " <<
                scs.subsession->clientPortNum() << "-" <<
                scs.subsession->clientPortNum()+1 << ")\n";

            if (!handlers::addSubsessionSink(env, scs.subsession)){
                utils::errorMsg("Failed linking subsession and QueueSink");
                scs.subsession->deInitiate();
            }

            if(!(queueSink = dynamic_cast<QueueSink *>(scs.subsession->sink))){
                utils::errorMsg("Failed to initiate subsession sink");
                scs.subsession->deInitiate();
            }

            if (!scs.addWriterToMngr(queueSink->getPort(), queueSink->getWriter())){
                utils::errorMsg("Failed adding writer in SourceManager");
                scs.subsession->deInitiate();
            }

            scs.sessionTimeoutParameter = rtspClient->sessionTimeoutParameter();

        } while (0);
        delete[] resultString;

        setupNextSubsession(rtspClient);
    }

    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
    {
        Boolean success = False;

        do {
            UsageEnvironment& env = rtspClient->envir();
            StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

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

            scs.sessionTimeoutBrokenServerTask = NULL;
            unsigned sessionTimeout = scs.sessionTimeoutParameter == 0 ? 60/*default*/ : scs.sessionTimeoutParameter;
            unsigned secondsUntilNextKeepAlive = sessionTimeout <= 5 ? 1 : sessionTimeout - 5;
            scs.sessionTimeoutBrokenServerTask
                        = env.taskScheduler().scheduleDelayedTask(secondsUntilNextKeepAlive*1000000,
                    		 (TaskFunc*)checkSessionTimeoutBrokenServer, rtspClient);

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
        StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

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

    void getOptions(RTSPClient* rtspClient, RTSPClient::responseHandler* afterFunc)
    {
        rtspClient->sendOptionsCommand(afterFunc/*, Authenticator*/);
    }

    void getParameterCommand(RTSPClient* rtspClient, RTSPClient::responseHandler* afterFunc)
    {
        StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());
        rtspClient->sendGetParameterCommand(*scs.session, afterFunc, /*extraParams*/NULL/*, Authenticator*/);
    }

    void streamTimerHandler(void* clientData)
    {
        ExtendedRTSPClient* rtspClient = (ExtendedRTSPClient*)clientData;
        StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

        scs.streamTimerTask = NULL;
        shutdownStream(rtspClient);
    }

    void checkSessionTimeoutBrokenServer(void* clientData)
    {
        ExtendedRTSPClient* rtspClient = (ExtendedRTSPClient*)clientData;
        UsageEnvironment& env = rtspClient->envir();
        StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());

        if (!scs.sendKeepAlivesToBrokenServers) return;

        if (scs.sessionTimeoutBrokenServerTask != NULL) {
            getParameterCommand(rtspClient, NULL);
        }

        unsigned sessionTimeout = scs.sessionTimeoutParameter == 0 ? DEFAULT_SESSION_TIMEOUT : scs.sessionTimeoutParameter;
        unsigned secondsUntilNextKeepAlive = sessionTimeout <= 5 ? 1 : sessionTimeout - 5;

        scs.sessionTimeoutBrokenServerTask
            = env.taskScheduler().scheduleDelayedTask(secondsUntilNextKeepAlive*1000000,
        		 (TaskFunc*)checkSessionTimeoutBrokenServer, rtspClient);
    }

    void shutdownStream(RTSPClient* rtspClient)
    {
        UsageEnvironment& env = rtspClient->envir();
        StreamClientState& scs = *(((ExtendedRTSPClient*)rtspClient)->getScs());


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

    //TODO: static method of SourceManager?
    bool addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession)
    {
        int wId;
        QueueSink *sink;
        Writer *writer;
        FramedFilter* filter = NULL;

        writer = new Writer();
        wId = subsession->clientPortNum();
        sink = QueueSink::createNew(env, writer, wId);

        if (sink == NULL){
            utils::errorMsg("Error creating subsession sink");
            delete writer;
            return false;
        }

        subsession->sink = sink;
        
        if (strcmp(subsession->codecName(), "H264") == 0 || strcmp(subsession->codecName(), "H265") == 0) {
            filter = H264VideoSdpParser::createNew(env, subsession->readSource(), subsession->fmtp_spropparametersets());
        }

        if (filter) {
            subsession->addFilter(filter);
        }

        subsession->sink->startPlaying(*(subsession->readSource()), handlers::subsessionAfterPlaying, subsession);

        if (subsession->rtcpInstance() != NULL) {
            subsession->rtcpInstance()->setByeHandler(handlers::subsessionByeHandler, subsession);
        }

        return true;
    }

    std::string modifySessionName(std::string sdp, std::string sessionName)
    {
        std::string newSdp;
        std::string line;
        std::string prefix = "s=";
        std::istringstream iss(sdp);
        for (; std::getline(iss, line);)
        {
            if(0 == line.find(prefix)) {
                line = prefix + sessionName + "\n";
            }
            newSdp += line;
        }
        return newSdp;
    }

};
