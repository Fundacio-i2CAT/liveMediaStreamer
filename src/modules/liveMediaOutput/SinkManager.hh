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

#include "QueueSource.hh"
#include "../../Filter.hh"
#include "../../IOInterface.hh"
#include "Connection.hh"

#include <map>
#include <string>

#define RTSP_PORT 8554
#define MAX_VIDEO_FRAME_SIZE 1024*1024
#define MANUAL_CLIENT_SESSION_ID 1
#define INIT_SEGMENT 0


class SinkManager : public TailFilter {
private:
    SinkManager(int readersNum = MAX_READERS);
    ~SinkManager();

public:
    static SinkManager* getInstance();
    static void destroyInstance();

    bool addSession(std::string id, std::vector<int> readers,
                    std::string info = "", std::string desc = "");

    /**
    * Adds an RTP connection
    * @param readers Readers associated to the connection (some connection support multiple readers)
    * @param id Connection Id, which must be unique for each one
    * @param ip Destination IP
    * @param port Destination port
    * @param txFormat Transmission format which can be STD_RTP (no container), MPEGTS, Destination port
    * @return True if succeded and false if not
    */
    bool addRTPConnection(std::vector<int> readers, int id, std::string ip, int port, TxFormat txFormat);
    bool addDASHConnection(int reader, unsigned id, std::string fileName, std::string quality,
                           bool reInit, uint32_t segmentTime,
                           uint32_t initSegment, uint32_t fps);

    ServerMediaSession* getSession(std::string id);
    bool publishSession(std::string id);
    bool removeSession(std::string id);
    bool removeSessionByReaderId(int readerId);
    bool deleteReader(int id);

    void stop();

    UsageEnvironment* envir() {return env;}

private:
    bool addStdRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    bool addUltraGridRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    bool addMpegTsRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    void initializeEventMap();
    void addSessionEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void addRTPConnectionEvent(Jzon::Node* params, Jzon::Object &outputNode);
    Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false);

    size_t processFrame(bool removeFrame = false);

    ServerMediaSubsession *createSubsessionByReader(int readerId);
    ServerMediaSubsession *createVideoMediaSubsession(VCodecType codec, int readerId);
    ServerMediaSubsession *createAudioMediaSubsession(ACodecType codec,
                                                      unsigned channels,
                                                      unsigned sampleRate,
                                                      SampleFmt sampleFormat, int readerId);
    void createVideoQueueSource(VCodecType codec, Reader *reader, int readerId);
    void createAudioQueueSource(ACodecType codec, Reader *reader, int readerId);
    void doGetState(Jzon::Object &filterNode);

    static SinkManager* mngrInstance;
    std::map<std::string, ServerMediaSession*> sessionList;
    std::map<int, StreamReplicator*> replicators;
    std::map<int, Connection*> connections;
    UsageEnvironment* env;
    uint8_t watch;

    RTSPServer* rtspServer;
};

#endif
