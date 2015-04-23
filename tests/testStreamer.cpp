/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2015, Live Networks, Inc.  All rights reserved
// A test program that reads a H264 file and/or and OPUS file
// and streams through RTP
// main program

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <string>

#include "LoopByteStreamFileSource.hh"

#define TRANSPORT_PACKET_SIZE 188
#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7
// The product of these two numbers must be enough to fit within a network packet

UsageEnvironment* env;
FramedSource* videoSource;
std::string vfile = "";
std::string afile = "";
Boolean   thereIsVideo = false;
Boolean   thereIsAudio = false;
RTPSink* videoSink;

struct in_addr destinationAddress;
unsigned short rtpPortNum = 1024; //let's assume this is the lowest port to bind at server side
const unsigned char ttl = 7; // low, in case routers don't admin scope

OggFile* oggFile;
OggDemux* oggDemux;
unsigned numTracks;

// A structure representing the state of an OGG/OPUS track:
struct TrackState {
  u_int32_t trackNumber;
  FramedSource* source;
  RTPSink* sink;
  RTCPInstance* rtcp;
};
TrackState* trackState;

unsigned int vport = 0;
unsigned int aport = 0;

void play(); // forward
void onOggFileCreation(OggFile* newFile, void* clientData); // forward

int main(int argc, char** argv) {
    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    OutPacketBuffer::maxSize = 1000000;

    std::string address = "";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-d")==0) {
            address = argv[i+1];
            *env << "destination address: " << address.c_str() << "\n";
        } else if (strcmp(argv[i],"-v")==0) {
            vport = std::stoi(argv[i+1]);
            *env << "video destination port: " << vport << "\n";
        } else if (strcmp(argv[i],"-vf")==0) {
            vfile = argv[i+1];
            *env << "video file to stream: " << vfile.c_str() << "\n";
        } else if (strcmp(argv[i],"-a")==0) {
            aport = std::stoi(argv[i+1]);
            *env << "audio destination port: " << aport << "\n";
        } else if (strcmp(argv[i],"-af")==0) {
            afile = argv[i+1];
            *env << "audio file to stream: " << afile.c_str() << "\n";
        }
    }

    if (vfile.length() > 0 && vport != 0) {
        thereIsVideo = true;
    } else {
        *env << "NO video file detected... continuing anyway...\n";
    }

    if (afile.length() > 0 && aport != 0) {
        thereIsAudio = true;
    } else {
        *env << "NO audio file detected... continuing anyway...\n";
    }

    if (address.length() == 0 || (!thereIsVideo && !thereIsAudio)){
        *env << "Usage: -d <IPAddress> [-v <portNum> -vf <H264 inputFile>] [-a <portNum> -af <OPUS inputFile>]" << "\n";
        return 1;
    }

    // Create 'groupsocks' for RTP and RTCP:
    destinationAddress.s_addr = our_inet_addr(address.c_str());

    const unsigned estimatedSessionBandwidth = 5000; // in kbps; for RTCP b/w share
    const unsigned maxCNAMElen = 100;
    unsigned char CNAME[maxCNAMElen+1];
    gethostname((char*)CNAME, maxCNAMElen);
    CNAME[maxCNAMElen] = '\0'; // just in case

    if(thereIsVideo){
        Groupsock *vRtpGroupsock = NULL;
        Groupsock *vRtcpGroupsock = NULL;

        for(;;rtpPortNum+=2){
            vRtpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum), ttl);
            vRtcpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum + 1), ttl);
            if (vRtpGroupsock->socketNum() < 0 || vRtcpGroupsock->socketNum() < 0) {
                delete vRtpGroupsock;
                delete vRtcpGroupsock;
                continue; // try again
            }
            break;
        }
        rtpPortNum+=2;

        vRtpGroupsock->changeDestinationParameters(destinationAddress, Port(vport), ttl);
        vRtcpGroupsock->changeDestinationParameters(destinationAddress, Port(vport + 1), ttl);

        // Create an appropriate 'RTP sink' from the RTP 'groupsock':
        videoSink =
            H264VideoRTPSink::createNew(*env, vRtpGroupsock, 96);

        // Create (and start) a 'RTCP instance' for this RTP sink:
        RTCPInstance::createNew(*env, vRtcpGroupsock,
                    estimatedSessionBandwidth, CNAME,
                    videoSink, NULL /* we're a server */, False);
        // Note: This starts RTCP running automatically
    }

    if(thereIsAudio){
        // Arrange to create an "OggFile" object for the specified file.
        // (Note that this object is not created immediately, but instead via a callback.)
        OggFile::createNew(*env, afile.c_str(), onOggFileCreation, NULL);
    }

    // Finally, start the streaming:
    *env << "Beginning streaming...\n";
    play();

    env->taskScheduler().doEventLoop(); // does not return

    return 0; // only to prevent compiler warning
}

void afterPlaying(void* /*clientData*/) {

    if(thereIsVideo){
        *env << "...done reading from video file\n";
        videoSink->stopPlaying();
        Medium::close(videoSource);
        // Note that this also closes the input file that this source read from.
    }
    if(thereIsAudio){
        *env << "...done reading from audio file\n";
        // Note that this also closes the input file that this source read from.
        unsigned i;
        for (i = 0; i < numTracks; ++i) {
            if (trackState[i].sink != NULL) trackState[i].sink->stopPlaying();
            Medium::close(trackState[i].source); trackState[i].source = NULL;
        }
        // Create a new demultiplexor from our Ogg file, then new data sources for each track:
        oggDemux = oggFile->newDemux();
        for (i = 0; i < numTracks; ++i) {
            if (trackState[i].trackNumber != 0) {
                FramedSource* baseSource
                    = oggDemux->newDemuxedTrack(trackState[i].trackNumber);

                unsigned estBitrate, numFiltersInFrontOfTrack;
                trackState[i].source
                    = oggFile->createSourceForStreaming(baseSource, trackState[i].trackNumber,
    			         estBitrate, numFiltersInFrontOfTrack);
            }
        }
    }

    play();
}

void play() {
    int reset = 0;
    if(thereIsVideo){
        // Open the input file as a 'byte-stream file source':
        LoopByteStreamFileSource* vFileSource
            = LoopByteStreamFileSource::createNew(*env, vfile.c_str(), reset);
        if (vFileSource == NULL) {
            *env << "Unable to open video file \"" << vfile.c_str()
                 << "\" as a byte-stream video file source\n";
            exit(1);
        }

        // Create a 'framer' for the input source (to give us proper inter-packet gaps):
        //videoSource = MPEG2TransportStreamFramer::createNew(*env, vFileSource);
        videoSource = H264VideoStreamFramer::createNew(*env, vFileSource);

        // Finally, start playing:
        *env << "Beginning to read from video file...\n";
        videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
    }
    if(thereIsAudio){
        *env << "Beginning to read from audio file...\n";
        // Start playing each track's RTP sink from its corresponding source:
        for (unsigned i = 0; i < numTracks; ++i) {
            if (trackState[i].sink != NULL && trackState[i].source != NULL) {
                trackState[i].sink->startPlaying(*trackState[i].source, afterPlaying, NULL);
            }
        }
    }
}

void onOggFileCreation(OggFile* newFile, void* clientData) {
    oggFile = newFile;

    // Create a new demultiplexor for the file:
    oggDemux = oggFile->newDemux();

    // Create source streams, "RTPSink"s, and "RTCPInstance"s for each preferred track;
    const unsigned maxCNAMElen = 100;
    unsigned char CNAME[maxCNAMElen+1];
    gethostname((char*)CNAME, maxCNAMElen);
    CNAME[maxCNAMElen] = '\0'; // just in case

    numTracks = oggFile->numTracks();

    trackState = new TrackState[numTracks];
    for (unsigned i = 0; i < numTracks; ++i) {
        u_int32_t trackNumber;
        FramedSource* baseSource = oggDemux->newDemuxedTrack(trackNumber);
        trackState[i].trackNumber = trackNumber;

        unsigned estBitrate, numFiltersInFrontOfTrack;
        trackState[i].source = oggFile
          ->createSourceForStreaming(baseSource, trackNumber, estBitrate, numFiltersInFrontOfTrack);
        trackState[i].sink = NULL; // by default; may get changed below
        trackState[i].rtcp = NULL; // ditto

        if (trackState[i].source != NULL) {
            Groupsock* aRtpGroupsock;
            Groupsock* aRtcpGroupsock;
            for(;;rtpPortNum+=2){
                aRtpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum), ttl);
                aRtcpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum + 1), ttl);
                if (aRtpGroupsock->socketNum() < 0 || aRtcpGroupsock->socketNum() < 0) {
                    delete aRtpGroupsock;
                    delete aRtcpGroupsock;
                    continue; // try again
                }
                break;
            }

            aRtpGroupsock->changeDestinationParameters(destinationAddress, Port(aport), ttl);
            aRtcpGroupsock->changeDestinationParameters(destinationAddress, Port(aport + 1), ttl);
            aport += 2;

            trackState[i].sink
            = oggFile->createRTPSinkForTrackNumber(trackNumber, aRtpGroupsock, 97+i);
            if (trackState[i].sink != NULL) {
                if (trackState[i].sink->estimatedBitrate() > 0) {
                    estBitrate = trackState[i].sink->estimatedBitrate(); // hack
                }
                trackState[i].rtcp
                    = RTCPInstance::createNew(*env, aRtcpGroupsock, estBitrate, CNAME,
                		    trackState[i].sink, NULL /* we're a server */,
                		    True /* we're a SSM source */);
                // Note: This starts RTCP running automatically
            }
        }
    }
    play();
}
