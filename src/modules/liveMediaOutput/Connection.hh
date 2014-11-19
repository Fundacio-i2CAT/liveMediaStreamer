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

class Connection {
    
public:
    void startPlaying();
    void stopPlaying();
    ~Connection();
    
protected:
    Connection(UsageEnvironment* env, std::string ip, 
               unsigned port, FramedSource *source);
    Connection(UsageEnvironment* env, std::string fileName, FramedSource *source);
    static void afterPlaying(void* clientData);
    
    
    UsageEnvironment* fEnv;
    std::string fIp;
    unsigned fPort;
    std::string fFileName;
    FramedSource *fSource;
    
    struct in_addr destinationAddress;
    RTPSink *sink;
    DashFileSink *outputVideoFile;
    FileSink *outputAudioFile;
    RTCPInstance* rtcp;
    Groupsock *rtpGroupsock;
    Groupsock *rtcpGroupsock;
};

//RAW RTP CONNECTIONS

class VideoConnection : public Connection {   
public:
    VideoConnection(UsageEnvironment* env, 
                    std::string ip, unsigned port, 
                    FramedSource *source, VCodecType codec);

private:
    VCodecType fCodec;
};


class AudioConnection : public Connection {
public:
    AudioConnection(UsageEnvironment* env, std::string ip, unsigned port, 
                    FramedSource *source, ACodecType codec,
                    unsigned channels, unsigned sampleRate,
                    SampleFmt sampleFormat);
    
private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
};

//ULTRAGRID RTP CONNECTIONS

class UltraGridVideoConnection : public Connection {   
public:
    UltraGridVideoConnection(UsageEnvironment* env, 
                             std::string ip, unsigned port, 
                             FramedSource *source);

};


class UltraGridAudioConnection : public Connection {
public:
    UltraGridAudioConnection(UsageEnvironment* env, 
                             std::string ip, unsigned port, 
                             FramedSource *source);
};

//DASH RTP CONNECTIONS

class DashVideoConnection : public Connection {   
public:
    DashVideoConnection(UsageEnvironment* env, std::string fileName, 
                        FramedSource *source, VCodecType codec, 
                        std::string quality, uint32_t fps = FRAME_RATE, 
                        bool reInit = false, uint32_t segmentTime = SEGMENT_TIME, 
                        uint32_t initSegment = INIT_SEGMENT);

private:
    VCodecType fCodec;
    bool fReInit;
    uint32_t fFps;
    uint32_t fSegmentTime;
    uint32_t fInitSegment;
};

class DashAudioConnection : public Connection {
public:
    DashAudioConnection(UsageEnvironment* env, std::string fileName, 
                        FramedSource *source, ACodecType codec,
                        unsigned channels, unsigned sampleRate,
                        SampleFmt sampleFormat, std::string quality, 
                        bool reInit = false, uint32_t segmentTime = SEGMENT_TIME, 
                        uint32_t initSegment = INIT_SEGMENT);
    
private:
    ACodecType fCodec;
    unsigned fChannels;
    unsigned fSampleRate;
    SampleFmt fSampleFormat;
    bool fReInit;
    uint32_t fSegmentTime;
    uint32_t fInitSegment;
};

#endif