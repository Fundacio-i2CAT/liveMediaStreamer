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
// A test program that reads a MPEG-2 Transport Stream file,
// and streams it using RTP
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
std::string file = "";
RTPSink* videoSink;

void play(); // forward

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

    OutPacketBuffer::maxSize = 1000000;
  
    std::string address = "";
    unsigned int port = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-a")==0) {
            address = argv[i+1];
            *env << "destination adress: " << address.c_str() << "\n";
        } else if (strcmp(argv[i],"-p")==0) {
            port = std::stoi(argv[i+1]);
            *env << "destination port: " << port << "\n";
        } else if (strcmp(argv[i],"-f")==0) {
            file = argv[i+1];
            *env << "file to stream: " << file.c_str() << "\n";
        }
    }
    
    if (file.length() == 0 || address.length() == 0 || port == 0){
        *env << "Usage: -a <IPAddress> -p <portNum> -f <inputFile>" << "\n";
        return 1;
    }
  
  // Create 'groupsocks' for RTP and RTCP:
  unsigned short rtpPortNum = 1024; //let's assume this is the lowest port to bind at server side
  const unsigned char ttl = 7; // low, in case routers don't admin scope

  struct in_addr destinationAddress;
  destinationAddress.s_addr = our_inet_addr(address.c_str());
  
  Groupsock *rtpGroupsock = NULL;
  Groupsock *rtcpGroupsock = NULL;

  for(;;rtpPortNum+=2){
    rtpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum), ttl);
    rtcpGroupsock = new Groupsock(*env, destinationAddress, Port(rtpPortNum + 1), ttl);
    if (rtpGroupsock->socketNum() < 0 || rtcpGroupsock->socketNum() < 0) {
            delete rtpGroupsock;
            delete rtcpGroupsock;
            continue; // try again
    }
    break;
  }
  
  rtpGroupsock->changeDestinationParameters(destinationAddress, Port(port), ttl);
  rtcpGroupsock->changeDestinationParameters(destinationAddress, Port(port + 1), ttl);

  // Create an appropriate 'RTP sink' from the RTP 'groupsock':
  videoSink =
    SimpleRTPSink::createNew(*env, rtpGroupsock, 33, 90000, "video", "MP2T",
			     1, True, False /*no 'M' bit*/);
     

  // Create (and start) a 'RTCP instance' for this RTP sink:
  const unsigned estimatedSessionBandwidth = 5000; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  gethostname((char*)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case
  RTCPInstance::createNew(*env, rtcpGroupsock,
			    estimatedSessionBandwidth, CNAME,
			    videoSink, NULL /* we're a server */, False);
  // Note: This starts RTCP running automatically

  // Finally, start the streaming:
  *env << "Beginning streaming...\n";
  play();

  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}

void afterPlaying(void* /*clientData*/) {
  *env << "...done reading from file\n";

  videoSink->stopPlaying();
  Medium::close(videoSource);
  // Note that this also closes the input file that this source read from.

  play();
}

void play() {
    
  int reset = 0;
  unsigned const inputDataChunkSize
    = TRANSPORT_PACKETS_PER_NETWORK_PACKET*TRANSPORT_PACKET_SIZE;

  // Open the input file as a 'byte-stream file source':
  LoopByteStreamFileSource* fileSource
    = LoopByteStreamFileSource::createNew(*env, file.c_str(), reset, inputDataChunkSize);
  if (fileSource == NULL) {
    *env << "Unable to open file \"" << file.c_str()
         << "\" as a byte-stream file source\n";
    exit(1);
  }

  // Create a 'framer' for the input source (to give us proper inter-packet gaps):
  videoSource = MPEG2TransportStreamFramer::createNew(*env, fileSource);

  // Finally, start playing:
  *env << "Beginning to read from file...\n";
  videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
}
