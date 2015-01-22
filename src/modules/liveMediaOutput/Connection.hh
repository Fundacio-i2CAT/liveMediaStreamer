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
#include "DashFileSink.hh"

#define TTL 255
#define INITIAL_SERVER_PORT 6970

class Connection {
    
public:
    void startPlaying();
    void stopPlaying();
    virtual ~Connection();
    bool setup();
    
protected:
    Connection(UsageEnvironment* env, FramedSource *source);
    
    static void afterPlaying(void* clientData);
    virtual bool specificSetup() = 0;

    UsageEnvironment* fEnv;
    FramedSource* fSource;
    MediaSink* fSink;
};

////////////////////
// RTP CONNECTION //
////////////////////

class RTPConnection : public Connection {
    
public:
    virtual ~RTPConnection();
    
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

private:
    bool firstStepSetup();
    bool finalRTCPSetup();
};

/////////////////////
// DASH CONNECTION //
/////////////////////

class DASHConnection : public Connection {
    
public:
    virtual ~DASHConnection();
    
protected:
    DASHConnection(UsageEnvironment* env, FramedSource* source, std::string fileName,
                   std::string quality, bool reInit, uint32_t segmentTime, uint32_t initSegment);
    bool specificSetup();
    virtual bool additionalSetup() = 0; 
    
    std::string fFileName;
    std::string quality;
    bool fReInit;
    uint32_t fSegmentTime;
    uint32_t fInitSegment;
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

class MpegTsConnection : public RTPConnection {   
public:
    MpegTsConnection(UsageEnvironment* env,
                     std::string ip, unsigned port);
    bool addVideoSource(FramedSource* source, VCodecType codec);
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
