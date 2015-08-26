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

#include "../../Filter.hh"
#include "../../StreamInfo.hh"
#include "Handlers.hh"
#include "QueueSink.hh"

#include <map>
#include <list>
#include <functional>
#include <string>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>


#define PROTOCOL "RTP"
#define DEFAULT_STATS_TIME_INTERVAL 1000 // 1 second

class SourceManager;
class SCSSubsessionStats;

class StreamClientState {
public:
    StreamClientState(std::string id_, SourceManager *const  manager);
    virtual ~StreamClientState();

    std::string getId(){return id;}

    bool addSinkToMngr(unsigned id, QueueSink* sink);

    bool addNewSubsessionStats(size_t port, MediaSubsession* subsession);

    bool removeSubsessionStats(size_t port);

    SCSSubsessionStats* getSubsessionStats(size_t port);

    void scheduleNextStatsMeasurement(UsageEnvironment* env);

    std::map<int, SCSSubsessionStats*> getSCSSubsesionStatsMap() { return smsStats; };

public:
    SourceManager *const mngr;

    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
    TaskToken sessionTimeoutBrokenServerTask;
    TaskToken sessionStatsMeasurementTask;
    size_t statsMeasurementIntervalMS;
    size_t nextStatsMeasurementUSecs;
    bool sendKeepAlivesToBrokenServers;
    unsigned sessionTimeoutParameter;

private:
    std::string id;
    std::map<int, SCSSubsessionStats*> smsStats;
};

class Session {
public:
    static Session* createNewByURL(UsageEnvironment& env, std::string progName, std::string rtspURL, std::string id, SourceManager *const mngr);
    static Session* createNew(UsageEnvironment& env, std::string sdp, std::string id, SourceManager *const mngr);

    ~Session();

    std::string getId() {return scs->getId();};
    MediaSubsession* getSubsessionByPort(int port);
    StreamClientState* getScs() {return scs;};

    bool initiateSession();

protected:
    Session(std::string id, SourceManager *const mngr);

    RTSPClient* client;
    StreamClientState *scs;
};

class SourceManager : public HeadFilter {
public:
    SourceManager(unsigned writersNum = MAX_WRITERS);
    ~SourceManager();

public:
    static std::string makeSessionSDP(std::string sessionName, std::string sessionDescription);
    static std::string makeSubsessionSDP(std::string mediumName, std::string protocolName,
                                  unsigned int RTPPayloadFormat,
                                  std::string codecName, unsigned int bandwidth,
                                  unsigned int RTPTimestampFrequency,
                                  unsigned int clientPortNum = 0,
                                  unsigned int channels = 0);

    bool addSession(Session* session);
    bool removeSession(std::string id);

    Session* getSession(std::string id);
    int getWriterID(unsigned int port);

    UsageEnvironment* envir() {return env;}

private:
    void initializeEventMap();
    friend bool handlers::addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession);
    
    void doGetState(Jzon::Object &filterNode);
    bool addSessionEvent(Jzon::Node* params);
    bool removeSessionEvent(Jzon::Node* params);

    friend bool Session::initiateSession();
    friend bool StreamClientState::addSinkToMngr(unsigned port, QueueSink* sink);
    bool addSink(unsigned port, QueueSink *sink);

    bool doProcessFrame(std::map<int, Frame*> &dFrames);
    void addConnection(int wId, MediaSubsession* subsession);

    static void* startServer(void *args);
    FrameQueue *allocQueue(ConnectionData cData);

    std::map<std::string, Session*> sessionMap;

    /* StreamInfo indexed by writerID */
    std::map<int, StreamInfo *> outputStreamInfos;
    std::map<int, QueueSink*> sinks;
    std::mutex sinksMtx;

    UsageEnvironment* env;
    BasicTaskScheduler0 *scheduler;
};

/*! It represents a SourceManager subsession statistics object. It contains the port (id of the subsession) and average, 
    minumum and maximum values of packet loss, bit rate and inter packet gap parameters, as well as the jitter. */

class SCSSubsessionStats {

public:
    /**
    * Class constructor
    * @param port as the subsession id
    */
    SCSSubsessionStats(size_t id_, RTPSource* src, struct timeval const& startTime);

    /**
    * Class destructor
    */
    ~SCSSubsessionStats();

    /**
    * Periodic subsession stat measurement from current input time since last time
    * @param current time to measure
    */
    void periodicStatMeasurement(struct timeval const& timeNow);
    
    /**
    * Getters of SCSSubsessionStats class attributes
    */
    size_t getId() { return id;};
    struct timeval getMeasurementStartTime() { return measurementStartTime; };
    struct timeval getMeasurementEndTime() { return measurementEndTime; };
    double getKbitsPerSecondMin() { return kbitsPerSecondMin; };
    double getKbitsPerSecondMax() { return kbitsPerSecondMax; };
    double getKBytesTotal() { return kBytesTotal; };
    double getPacketLossFractionMin() { return packetLossFractionMin; };
    double getPacketLossFractionMax() { return packetLossFractionMax; };
    unsigned getTotNumPacketsReceived() { return totNumPacketsReceived; };
    unsigned getTotNumPacketsExpected() { return totNumPacketsExpected; };
    unsigned getMinInterPacketGapUS() { return minInterPacketGapUS; };
    unsigned getMaxInterPacketGapUS() { return maxInterPacketGapUS; };
    struct timeval getTotalGaps() { return totalGaps; };
    size_t getJitter() { return jitter; };
    size_t getMaxJitter() { return maxJitter; };
    size_t getMinJitter() { return minJitter; };
    RTPSource* getRTPSource() { return fSource; };

private:
    const size_t id;    // port
    RTPSource* fSource;
    struct timeval measurementStartTime, measurementEndTime;
    double kbitsPerSecondMin, kbitsPerSecondMax;
    double kBytesTotal;
    double packetLossFractionMin, packetLossFractionMax;
    unsigned totNumPacketsReceived, totNumPacketsExpected;
    unsigned minInterPacketGapUS, maxInterPacketGapUS;
    struct timeval totalGaps;
    // Estimate of the statistical variance of the 
    // RTP data interarrival time to be inserted in 
    // the interarrival jitter field of reception reports (in microseconds).
    size_t jitter;
    size_t maxJitter;
    size_t minJitter;
};

#endif
