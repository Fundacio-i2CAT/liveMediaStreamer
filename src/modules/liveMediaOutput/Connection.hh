/*
 *  Connection.hh - Class that represents a RTP transmission.
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 *
 */

#ifndef _CONNECTION_HH
#define _CONNECTION_HH

#include <string>
#include <vector>
#include <map>
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include <Groupsock.hh>

#include "../../Types.hh"
#include "MPEGTSQueueServerMediaSubsession.hh"

#define TTL 255
#define INITIAL_SERVER_PORT 6970
#define DEFAULT_STATS_TIME_INTERVAL 1000 // 1 second

class ConnRTCPInstance;
class ConnectionSubsessionStats;
class MPEGTSQueueServerMediaSubsession;

/*! Each connection represents a RTP/RTSP transmission. It is an interface to different specific connections
    so it cannot be instantiated */

class Connection {

public:
    /**
    * Class destructor. It stops the transmission if still playing
    */
    virtual ~Connection();

    /**
    * It configures the connection and start the transmission
    * @return true if succeeded and false if not
    */
    bool setup();
    
    /**
    * It returns the list of associated readers of this connection
    * @return return a vector with the readers Ids in this connection
    */
    virtual std::vector<int> getReaders() = 0;
    
    /**
    * Stops transmitting this connection
    */
    virtual void stopPlaying() = 0;

    std::map<int, ConnRTCPInstance*> getConnectionRTCPInstanceMap() { return cRTCPInstances; };

    void addConnectionRTCPInstance(size_t id, ConnRTCPInstance* cri) { cRTCPInstances[id] = cri; };

    void deleteConnectionRTCPInstance(size_t id) { cRTCPInstances.erase(id); };

    UsageEnvironment* envir() { return fEnv; };

protected:
    Connection(UsageEnvironment* env);

    virtual bool startPlaying() = 0;
    static void afterPlaying(void* clientData);
    virtual bool specificSetup() = 0;
    
    UsageEnvironment* fEnv;

    std::map<int, ConnRTCPInstance*> cRTCPInstances;
};

////////////////////
// RTSP CONNECTION //
////////////////////

/*! It represents an RTSP connection, which is defined by a name, format and former readers */

class RTSPConnection : public Connection {

public:
    
    /**
    * Class destructor
    */
    RTSPConnection(UsageEnvironment* env, TxFormat txformat, RTSPServer* server,
                   std::string name_, std::string info = "", std::string desc = "");
    
    /**
    * Class destructor
    */
    ~RTSPConnection();
    
    /**
    * Adds a video source to the connection
    * @param codec represents the video codec of the source to add
    * @param replicator it is the replicator from where the source is created
    * @param readerId it is the id of the reader associated with this replicator
    * @return True if succeded and false if not
    */
    bool addVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId);
    
    /**
    * Adds an audio source to the connection
    * @param codec represents the audio codec of the source to add
    * @param replicator it is the replicator from where the source is created
    * @param channels it is the number of audio channels of this source
    * @param smapleRate it is the sample rate of this source
    * @param sampleFormat it is the format of this source samples
    * @param readerId it is the id of the reader associated with this replicator
    * @return True if succeded and false if not
    */
    bool addAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                            unsigned channels, unsigned sampleRate, 
                            SampleFmt sampleFormat, int readerId);
    
    /**
    * @return Return the connection URI
    */
    std::string getURI();
    
    /**
    * @return it returns the RTSPConnection name
    */
    std::string getName() const {return name;};
    
    /**
    * It returns the list of associated readers of this connection
    * @return return a vector with the readers Ids in this connection
    */
    std::vector<int> getReaders();
    
    /**
    * Stops transmitting this connection
    */
    void stopPlaying();

protected:

    bool startPlaying();
    bool specificSetup();

private:
    bool addRawVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId);
    
    bool addRawAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                            unsigned channels, unsigned sampleRate, 
                            SampleFmt sampleFormat, int readerId);
    
    bool addMPEGTSVideo(VCodecType codec, StreamReplicator* replicator, int readerId);
    
    bool addMPEGTSAudio(ACodecType codec, StreamReplicator* replicator, int readerId);
    
    ServerMediaSession* session;
    //TODO: this should be const
    RTSPServer* const rtspServer;
    std::string name;
    
    MPEGTSQueueServerMediaSubsession* subsession;
    
    TxFormat format;
    bool addedSub;
    bool started;
};

////////////////////
// RTP CONNECTION //
////////////////////

/*! It represents an RTP connection, which is defined by a destination IP and port. It is an interface so it
    cannot be instantiated */

class RTPConnection : public Connection {

public:
    /**
    * Class destructor
    */
    virtual ~RTPConnection();
    
    bool startPlaying();
    void stopPlaying();
    
    std::string getIP() const {return fIp;};
    unsigned getPort() const {return fPort;};

protected:
    RTPConnection(UsageEnvironment* env, FramedSource* source,
                  std::string ip, unsigned port);

    bool specificSetup();
    virtual bool additionalSetup() = 0;

    std::string fIp;
    unsigned fPort;
    struct in_addr destinationAddress;
    ConnRTCPInstance* rtcp;
    Groupsock *rtpGroupsock;
    Groupsock *rtcpGroupsock;
    
    FramedSource* fSource;
    MediaSink* fSink;

private:
    bool firstStepSetup();
    bool finalRTCPSetup();
};

/////////////////////////
// RAW RTP CONNECTIONS //
/////////////////////////

class VideoConnection : public RTPConnection {
public:
    VideoConnection(UsageEnvironment* env, FramedSource *source,
                    std::string ip, unsigned port, VCodecType codec, int readerId);
    
    std::vector<int> getReaders();
    
protected:
    bool additionalSetup();

private:
    VCodecType fCodec;
    
    int reader;
};


class AudioConnection : public RTPConnection {
public:
    AudioConnection(UsageEnvironment* env, FramedSource *source,
                    std::string ip, unsigned port, ACodecType codec,
                    unsigned channels, unsigned sampleRate, SampleFmt sampleFormat, int readerId);
    
    std::vector<int> getReaders();
    
protected:
    bool additionalSetup();

private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
    
    int reader;
};

///////////////////////////////
// ULTRAGRID RTP CONNECTIONS //
///////////////////////////////

class UltraGridVideoConnection : public RTPConnection {
public:
    UltraGridVideoConnection(UsageEnvironment* env,
                             FramedSource *source,
                             std::string ip, unsigned port, VCodecType codec, int readerId);

    std::vector<int> getReaders();
    
protected:
    bool additionalSetup();

private:
    VCodecType fCodec;
    
    int reader;
};


class UltraGridAudioConnection : public RTPConnection {
public:
    UltraGridAudioConnection(UsageEnvironment* env,
                             FramedSource *source,
                             std::string ip, unsigned port, ACodecType codec,
                             unsigned channels, unsigned sampleRate, SampleFmt sampleFormat, int readerId);
    
    std::vector<int> getReaders();
    
protected:
    bool additionalSetup();

private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
    
    int reader;
};

////////////////////////
// MPEG-TS CONNECTION //
////////////////////////

/*! It represents an RTP connection using MPEGTS container to mux the streams.
*   It is limited to one video stream and/or one audio stream.
*   Only H264 (for video) and AAC (for audio) codecs are supported
*/

class MpegTsConnection : public RTPConnection {
public:
    /**
    * Class constructor
    * @param env Live555 UsageEnvironement
    * @param ip Destination IP
    * @param port Destination port
    */
    MpegTsConnection(UsageEnvironment* env, std::string ip, unsigned port);

    /**
    * Adds a video source to the connection, which will be muxed in MPEGTS packets
    * @param source Stream source, which must be a children of Live555 FramedSource class
    * @param codec Video stream codec. Only H264 is supported
    * @return True if succeeded and false if not
    */
    bool addVideoSource(FramedSource* source, VCodecType codec, int readerId);

    /**
    * Adds an audio source to the connection, which will be muxed in MPEGTS packets
    * @param source Stream source, which must be a children of Live555 FramedSource class
    * @param codec Audio stream codec. Only AAC is supported
    * @return True if succeeded and false if not
    */
    bool addAudioSource(FramedSource* source, ACodecType codec, int readerId);
    
    std::vector<int> getReaders();

protected:
    bool additionalSetup();

private:
    MPEG2TransportStreamFromESSource* tsFramer;
    
    int audioReader;
    int videoReader;
};

//////////////////////////////
// RTCP CONNECTION INSTANCE //
//////////////////////////////

/*! It represents an RTCP Instance of the RTP connection.
*   It measures network statistics for each connections' subsession.
*/
class ConnRTCPInstance : public RTCPInstance {
public:
    /**
    * Create new ConnRTCPInstance object
    */
    static ConnRTCPInstance* createNew(Connection* conn, UsageEnvironment* env, Groupsock* RTCPgs,
                                    unsigned totSessionBW,
                                    RTPSink* sink);
    /**
    * Class destructor
    */
    ~ConnRTCPInstance();

    RTPSink* getRTPSink() { return fSink; };
    
    void scheduleNextConnStatMeasurement();

    void periodicStatMeasurement(struct timeval const& timeNow);

    void setId(size_t _id) { id = _id; };

    size_t getId() { return id; };

    u_int32_t getSSRC() { return SSRC; };

    unsigned getCurrentNumBytes () { return currentNumBytes; };

    double getCurrentElapsedTime () { return currentElapsedTime; };

    u_int8_t getPacketLossRatio() { return packetLossRatio; };

    size_t getRoundTripDelay() { return roundTripDelay; };

    size_t getJitter() { return jitter; };

private:
    ConnRTCPInstance(Connection* conn, UsageEnvironment* env, Groupsock* RTPgs, unsigned totSessionBW,
                        unsigned char const* cname, RTPSink* sink);
    
    size_t id;
    u_int32_t SSRC;

    Connection* fConn;
    RTPSink* fSink;

    TaskToken connectionStatsMeasurementTask;
    size_t statsMeasurementIntervalMS;
    size_t nextStatsMeasurementUSecs;
    unsigned currentNumBytes;
    double currentElapsedTime;
    u_int8_t packetLossRatio;
    size_t roundTripDelay, jitter;
};

#endif