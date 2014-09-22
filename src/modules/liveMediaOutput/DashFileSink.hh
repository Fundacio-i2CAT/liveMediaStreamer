#ifndef _DASH_FILE_SINK_HH
#define _DASH_FILE_SINK_HH

#ifndef _MEDIA_SINK_HH
#include <MediaSink.hh>
#endif
#include "DashSegmenterVideoSource.hh"
#include "DashSegmenterAudioSource.hh"
#define ONLY_VIDEO 0
#define ONLY_AUDIO 1
#define VIDEO_AUDIO 2

class DashFileSink: public MediaSink {
public:
  static DashFileSink* createNew(UsageEnvironment& env, char const* fileName,
			     unsigned bufferSize = 20000,
			     Boolean oneFilePerFrame = False, char const* quality = NULL, unsigned segmentNumber = 0, char const* extension = NULL, unsigned streamType = 0, bool eraseFiles = false);
  // "bufferSize" should be at least as large as the largest expected
  //   input frame.
  // "oneFilePerFrame" - if True - specifies that each input frame will
  //   be written to a separate file (using the presentation time as a
  //   file name suffix).  The default behavior ("oneFilePerFrame" == False)
  //   is to output all incoming data into a single file.

  virtual void addData(unsigned char const* data, unsigned dataSize,
		       struct timeval presentationTime);
  // (Available in case a client wants to add extra data to the output file)

protected:
  DashFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
	   char const* perFrameFileNamePrefix, char const* quality = NULL, unsigned segmentNumber = 0, char const* extension = NULL, unsigned streamType = 0, bool eraseFiles = false);
      // called only by createNew()
  virtual ~DashFileSink();

protected: // redefined virtual functions:
  virtual Boolean continuePlaying();

protected:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  virtual void afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime);

  FILE* fOutFid;
  unsigned char* fBuffer;
  unsigned fBufferSize;
  char* fPerFrameFileNamePrefix; // used if "oneFilePerFrame" is True
  char* fPerFrameFileNameBuffer; // used if "oneFilePerFrame" is True
  struct timeval fPrevPresentationTime;
  unsigned fSamePresentationTimeCounter;
  char* fQuality;
  unsigned fSegmentNumber;
  char* fExtension;
  unsigned fStreamType;
  bool fEraseFiles;
};

#endif
