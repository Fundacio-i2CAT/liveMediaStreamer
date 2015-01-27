#include <liveMedia.hh>
#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include "../src/modules/liveMediaOutput/DashSegmenterVideoSource.hh"

char const* inputFileName = "salida.h264";
char const* outputFileName = "./segments/out.m4v";

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


  DashSegmenterVideoSource* isoff = DashSegmenterVideoSource::createNew(*env, framer);


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
