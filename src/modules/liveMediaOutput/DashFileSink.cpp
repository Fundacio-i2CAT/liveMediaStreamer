#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
#include <io.h>
#include <fcntl.h>
#endif
#include "DashFileSink.hh"
#include "GroupsockHelper.hh"
#include "OutputFile.hh"


DashFileSink::DashFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		   char const* perFrameFileNamePrefix, char const* quality, unsigned segmentNumber, char const* extension, unsigned streamType, bool eraseFiles)
  : MediaSink(env), fOutFid(fid), fBufferSize(bufferSize), fSamePresentationTimeCounter(0), fSegmentNumber(segmentNumber), fStreamType(streamType), fEraseFiles(eraseFiles) {
  fBuffer = new unsigned char[bufferSize];
  if (perFrameFileNamePrefix != NULL) {
    fPerFrameFileNamePrefix = strDup(perFrameFileNamePrefix);
    fPerFrameFileNameBuffer = new char[strlen(perFrameFileNamePrefix) + 100];
  } else {
    fPerFrameFileNamePrefix = NULL;
    fPerFrameFileNameBuffer = NULL;
  }
  if (quality != NULL) {
    fQuality = strDup(quality);
  } else {
    fQuality = NULL;
  }
  if (extension != NULL) {
    fExtension = strDup(extension);
  } else {
    fExtension = NULL;
  }

  fPrevPresentationTime.tv_sec = ~0; fPrevPresentationTime.tv_usec = 0;
}

DashFileSink::~DashFileSink() {
  delete[] fPerFrameFileNameBuffer;
  delete[] fPerFrameFileNamePrefix;
  delete[] fBuffer;
  delete[] fQuality;
  delete[] fExtension;
  if (fOutFid != NULL) fclose(fOutFid);
}

DashFileSink* DashFileSink::createNew(UsageEnvironment& env, char const* fileName,
			      unsigned bufferSize, Boolean oneFilePerFrame, char const* quality, unsigned segmentNumber, char const* extension, unsigned streamType, bool eraseFiles) {
  do {
    FILE* fid;
    char const* perFrameFileNamePrefix;
    if (oneFilePerFrame) {
      // Create the fid for each frame
      fid = NULL;
      perFrameFileNamePrefix = fileName;
    } else {
      // Normal case: create the fid once
      fid = OpenOutputFile(env, fileName);
      if (fid == NULL) break;
      perFrameFileNamePrefix = NULL;
    }

    return new DashFileSink(env, fid, bufferSize, perFrameFileNamePrefix, quality, segmentNumber, extension, streamType, eraseFiles);
  } while (0);

  return NULL;
}

Boolean DashFileSink::continuePlaying() {
  if (fSource == NULL) return False;

  fSource->getNextFrame(fBuffer, fBufferSize,
			afterGettingFrame, this,
			onSourceClosure, this);

  return True;
}

void DashFileSink::afterGettingFrame(void* clientData, unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime,
				 unsigned /*durationInMicroseconds*/) {
  DashFileSink* sink = (DashFileSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void DashFileSink::addData(unsigned char const* data, unsigned dataSize,
		       struct timeval presentationTime) {
  if (fPerFrameFileNameBuffer != NULL && fOutFid == NULL) {
	 DashSegmenterVideoSource* dashSource = dynamic_cast<DashSegmenterVideoSource*> (fSource);
	if (dashSource->isInit()) {
		switch (fStreamType) {
		case ONLY_VIDEO:			
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_init.%s", fPerFrameFileNamePrefix, fQuality, "video", fExtension);
			break;
		case ONLY_AUDIO:
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_init.%s", fPerFrameFileNamePrefix, fQuality, "audio", fExtension);
			break;
		case VIDEO_AUDIO:
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_init.%s", fPerFrameFileNamePrefix, fQuality, "video_audio", fExtension);
			break;
		}
	}
	else {
		switch (fStreamType) {
		case ONLY_VIDEO:			
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_%u.%s", fPerFrameFileNamePrefix, fQuality, "video", fSegmentNumber++, fExtension);
			break;
		case ONLY_AUDIO:
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_%u.%s", fPerFrameFileNamePrefix, fQuality, "audio", fSegmentNumber++, fExtension);
			break;
		case VIDEO_AUDIO:
			sprintf(fPerFrameFileNameBuffer, "%s_%s_%s_%u.%s", fPerFrameFileNamePrefix, fQuality, "video_audio", fSegmentNumber++, fExtension);
			break;
		}
	}
    fOutFid = OpenOutputFile(envir(), fPerFrameFileNameBuffer);
	//TODO eraseFiles
  }

  // Write to our file:
#ifdef TEST_LOSS
  static unsigned const framesPerPacket = 10;
  static unsigned const frameCount = 0;
  static Boolean const packetIsLost;
  if ((frameCount++)%framesPerPacket == 0) {
    packetIsLost = (our_random()%10 == 0); // simulate 10% packet loss #####
  }

  if (!packetIsLost)
#endif
  if (fOutFid != NULL && data != NULL) {
    fwrite(data, 1, dataSize, fOutFid);
  }
}

void DashFileSink::afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime) {
  if (numTruncatedBytes > 0) {
    envir() << "DashFileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
	    << fBufferSize << ").  "
            << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
            << fBufferSize + numTruncatedBytes << "\n";
  }
  addData(fBuffer, frameSize, presentationTime);

  if (fOutFid == NULL || fflush(fOutFid) == EOF) {
    // The output file has closed.  Handle this the same way as if the input source had closed:
    if (fSource != NULL) fSource->stopGettingFrames();
    onSourceClosure();
    return;
  }

  if (fPerFrameFileNameBuffer != NULL) {
    if (fOutFid != NULL) { fclose(fOutFid); fOutFid = NULL; }
  }

  // Then try getting the next frame:
  continuePlaying();
}
