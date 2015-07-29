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
#include "../../StreamInfo.hh"
#include "../../IOInterface.hh"
#include "Connection.hh"

#include <map>
#include <string>
#include <liveMedia/liveMedia.hh>
#include <BasicUsageEnvironment.hh>

#define RTSP_PORT 8554
#define MAX_VIDEO_FRAME_SIZE 1024*1024*2 //2MB
#define MANUAL_CLIENT_SESSION_ID 1
#define INIT_SEGMENT 0


class SinkManager : public TailFilter {
public:
    static SinkManager* createNew(unsigned readersNum = MAX_READERS);
    
    ~SinkManager();

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
    
    /**
    * Adds an RTSP connection
    * @param readers Readers associated to the connection (some connection support multiple readers)
    * @param id Connection Id, which must be unique for each one
    * @param txFormat Transmission format which can be STD_RTP (no container) or MPEGTS
    * @param name name of the RTSP session used to generate the session URI
    * @param info information field of the session (optional)
    * @param desc description of the RTSP session (optional)
    * @return True if succeded and false if not
    */
    bool addRTSPConnection(std::vector<int> readers, int id, TxFormat txformat, 
                           std::string name, std::string info = "", std::string desc = "");
    
    /**
    * Removes the connection determined by the id
    * @param id Connection Id to delete
    * @return True if succeded, the connected with the given id has been stopped and deteled
    */
    bool removeConnection(int id);
    
    /**
     * Removes the connection determined by the id included in params field of Jzon::Node*
     * @return True if the connection was successfully removed, False otherwise
     */
    bool removeConnectionEvent(Jzon::Node* params);

    UsageEnvironment* envir() {return env;}

private:
    SinkManager(unsigned readersNum = MAX_READERS);
    
    bool readerInConnection(int rId);
    
    bool isGood() {return rtspServer != NULL;};
    
    bool addStdRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    bool addUltraGridRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    bool addMpegTsRTPConnection(std::vector<int> readers, int id, std::string ip, int port);
    void initializeEventMap();
    
    bool removeConnectionByReaderId(int readerId);
    
    bool addRTSPConnectionEvent(Jzon::Node* params);
    bool addRTPConnectionEvent(Jzon::Node* params);
    
    bool specificReaderConfig(int readerID, FrameQueue* queue);
    bool specificReaderDelete(int readerID);

    bool doProcessFrame(std::map<int, Frame*> &oFrames, std::vector<int> newFrames);
    void stop();

    bool addSubsessionByReader(RTSPConnection* connection, int readerId);

    bool createVideoQueueSource(VCodecType codec, int readerId);
    bool createAudioQueueSource(ACodecType codec, int readerId);
    void doGetState(Jzon::Object &filterNode);

    std::map<int, StreamReplicator*> replicators;
    std::map<int, QueueSource*> sources;
    std::map<int, Connection*> connections;

    RTSPServer* rtspServer;
    UsageEnvironment* env;
    BasicTaskScheduler0* scheduler;
};

#endif
