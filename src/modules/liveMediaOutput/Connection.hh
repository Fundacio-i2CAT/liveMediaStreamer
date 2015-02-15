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
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include <Groupsock.hh>
#include "../../Types.hh"

#define TTL 255
#define INITIAL_SERVER_PORT 6970

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

protected:
    Connection(UsageEnvironment* env);

    virtual bool startPlaying() = 0;
    virtual void stopPlaying() = 0;
    static void afterPlaying(void* clientData);
    virtual bool specificSetup() = 0;

    UsageEnvironment* fEnv;
};

////////////////////
// RTSP CONNECTION //
////////////////////

/*! It represents an RTSP connection, which is defined by a name, format and former readers */

class RTPSConnection : public Connection {

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
    
    bool addVideoSubsession(VCodecType codec, StreamReplicator* replicator, int readerId);
    
    bool addAudioSubsession(ACodecType codec, StreamReplicator* replicator,
                                        unsigned channels, unsigned sampleRate, 
                                        SampleFmt sampleFormat, int readerId);

protected:

    bool startPlaying();
    void stopPlaying();
    bool specificSetup();

private:
    ServerMediaSession* session;
    //TODO: this should be const
    RTSPServer* rtspServer
    std::string name;
    MPEG2TransportStreamFromESSource* tsFramer;
    TxFormat format;
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

protected:
    RTPConnection(UsageEnvironment* env, FramedSource* source,
                  std::string ip, unsigned port);

    bool specificSetup();
    virtual bool additionalSetup() = 0;

    std::string fIp;
    unsigned fPort;
    struct in_addr destinationAddress;
    RTCPInstance* rtcp;
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
                    std::string ip, unsigned port, VCodecType codec);
protected:
    bool additionalSetup();

private:
    VCodecType fCodec;
};


class AudioConnection : public RTPConnection {
public:
    AudioConnection(UsageEnvironment* env, FramedSource *source,
                    std::string ip, unsigned port, ACodecType codec,
                    unsigned channels, unsigned sampleRate, SampleFmt sampleFormat);
protected:
    bool additionalSetup();

private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
};

///////////////////////////////
// ULTRAGRID RTP CONNECTIONS //
///////////////////////////////

class UltraGridVideoConnection : public RTPConnection {
public:
    UltraGridVideoConnection(UsageEnvironment* env,
                             FramedSource *source,
                             std::string ip, unsigned port, VCodecType codec);

protected:
    bool additionalSetup();

private:
    VCodecType fCodec;
};


class UltraGridAudioConnection : public RTPConnection {
public:
    UltraGridAudioConnection(UsageEnvironment* env,
                             FramedSource *source,
                             std::string ip, unsigned port, ACodecType codec,
                             unsigned channels, unsigned sampleRate, SampleFmt sampleFormat);
protected:
    bool additionalSetup();

private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
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
    bool addVideoSource(FramedSource* source, VCodecType codec);

    /**
    * Adds an audio source to the connection, which will be muxed in MPEGTS packets
    * @param source Stream source, which must be a children of Live555 FramedSource class
    * @param codec Audio stream codec. Only AAC is supported
    * @return True if succeeded and false if not
    */
    bool addAudioSource(FramedSource* source, ACodecType codec);

protected:
    bool additionalSetup();

private:
    MPEG2TransportStreamFromESSource* tsFramer;
};

//////////////////////////
// DASH CONNECTIONS //
//////////////////////////

class DashVideoConnection : public DASHConnection {
public:
    DashVideoConnection(UsageEnvironment* env, FramedSource *source,
                        std::string fileName, std::string quality,
                        bool reInit, uint32_t segmentTime, uint32_t initSegment,
                        VCodecType codec, uint32_t fps);
protected:
    bool additionalSetup();

private:
    VCodecType fCodec;
    uint32_t fFps;
};

class DashAudioConnection : public DASHConnection {
public:
    DashAudioConnection(UsageEnvironment* env, FramedSource *source,
                        std::string fileName, std::string quality,
                        bool reInit, uint32_t segmentTime, uint32_t initSegment,
                        ACodecType codec, unsigned channels,
                        unsigned sampleRate, SampleFmt sampleFormat);
protected:
    bool additionalSetup();

private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
};

#endif
