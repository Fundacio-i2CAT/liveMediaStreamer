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
// Copyright (c) 1996-2014, Live Networks, Inc.  All rights reserved
// A program that converts a H.264 (Elementary Stream) video file into a Transport Stream file.
// main program

#include <liveMedia/liveMedia.hh>
#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include "../src/modules/liveMediaOutput/DashSegmenterVideoSource.hh"

//#define MAX_DAT 10*1024*1024 

char const* inputFileName = "in.h264";
char const* outputFileName = "out.m4v";

void afterPlaying(void* clientData); // forward

UsageEnvironment* env;

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  // Open the input file as a 'byte-stream file source':
  FramedSource* inputSource = ByteStreamFileSource::createNew(*env, inputFileName);
  if (inputSource == NULL) {
    *env << "Unable to open file \"" << inputFileName
	 << "\" as a byte-stream file source\n";
    exit(1);
  }

  // Create a 'framer' filter for this file source, to generate presentation times for each NAL unit:
  H264VideoStreamFramer* framer = H264VideoStreamFramer::createNew(*env, inputSource);

 


 // Then create a filter that packs the H.264 video data into a Transport Stream:
/*   MPEG2TransportStreamFromESSource* tsFrames = MPEG2TransportStreamFromESSource::createNew(*env);
  tsFrames->addNewVideoSource(framer, 5);
  
  // Open the output file as a 'file sink':
 MediaSink* outputSink = FileSink::createNew(*env, outputFileName);
  if (outputSink == NULL) {
    *env << "Unable to open file \"" << outputFileName << "\" as a file sink\n";
    exit(1);
  }

  // Finally, start playing:
  *env << "Beginning to read...\n";
  outputSink->startPlaying(*tsFrames, afterPlaying, NULL);*/




  DashSegmenterVideoSource* isoff = DashSegmenterVideoSource::createNew(*env, framer);
  //tsFrames->addNewVideoSource(framer, 5);

  MediaSink* outputSink = FileSink::createNew(*env, outputFileName, MAX_DAT, True);
  if (outputSink == NULL) {
    *env << "Unable to open file \"" << outputFileName << "\" as a file sink\n";
    exit(1);
  }

  // Finally, start playing:
  *env << "Beginning to read...\n";
  outputSink->startPlaying(*isoff, afterPlaying, NULL);




  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}

void afterPlaying(void* /*clientData*/) {
  *env << "Done reading.\n";
  *env << "Wrote output file: \"" << outputFileName << "\"\n";
  exit(0);
}
