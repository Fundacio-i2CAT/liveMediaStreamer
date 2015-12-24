/*
 *  LoopByteStreamFileSource.cpp - Class that loops the input file.
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
 *
 */

#include "LoopByteStreamFileSource.hh"
#include <Groupsock.hh>
#include <InputFile.hh>
#include <GroupsockHelper.hh>

////////// LoopByteStreamFileSource //////////

LoopByteStreamFileSource*
LoopByteStreamFileSource::createNew(UsageEnvironment& env, char const* fileName,
                                int& reset, unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
  FILE* fid = OpenInputFile(env, fileName);
  if (fid == NULL) return NULL;

  LoopByteStreamFileSource* newSource
    = new LoopByteStreamFileSource(env, fid, reset, preferredFrameSize, playTimePerFrame);
  newSource->fFileSize = GetFileSize(fileName, fid);

  return newSource;
}

LoopByteStreamFileSource*
LoopByteStreamFileSource::createNew(UsageEnvironment& env, FILE* fid,
                                int& reset, unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
  if (fid == NULL) return NULL;

  LoopByteStreamFileSource* newSource = new LoopByteStreamFileSource(env, fid, reset, preferredFrameSize, playTimePerFrame);
  newSource->fFileSize = GetFileSize(NULL, fid);

  return newSource;
}

void LoopByteStreamFileSource::seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream) {
  SeekFile64(fFid, (int64_t)byteNumber, SEEK_SET);

  fNumBytesToStream = numBytesToStream;
  fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void LoopByteStreamFileSource::seekToByteRelative(int64_t offset, u_int64_t numBytesToStream) {
  SeekFile64(fFid, offset, SEEK_CUR);

  fNumBytesToStream = numBytesToStream;
  fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void LoopByteStreamFileSource::seekToEnd() {
  SeekFile64(fFid, 0, SEEK_END);
}

LoopByteStreamFileSource::LoopByteStreamFileSource(UsageEnvironment& env, FILE* fid,
                                           int& reset, unsigned preferredFrameSize,
                                           unsigned playTimePerFrame)
  : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0), fReset(reset) {
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  makeSocketNonBlocking(fileno(fFid));
#endif

  // Test whether the file is seekable
  fFidIsSeekable = FileIsSeekable(fFid);
}

LoopByteStreamFileSource::~LoopByteStreamFileSource() {
  if (fFid == NULL) return;

#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
#endif

  CloseInputFile(fFid);
}

void LoopByteStreamFileSource::doGetNextFrame() {
  if (fReset != 0 || feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
    seekToByteAbsolute(0);
  }

#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  doReadFromFile();
#else
  if (!fHaveStartedReading) {
    // Await readable data from the file:
    envir().taskScheduler().turnOnBackgroundReadHandling(fileno(fFid),
               (TaskScheduler::BackgroundHandlerProc*)&fileReadableHandler, this);
    fHaveStartedReading = True;
  }
#endif
}

void LoopByteStreamFileSource::doStopGettingFrames() {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
  fHaveStartedReading = False;
#endif
}

void LoopByteStreamFileSource::fileReadableHandler(LoopByteStreamFileSource* source, int /*mask*/) {
  if (!source->isCurrentlyAwaitingData()) {
    source->doStopGettingFrames(); // we're not ready for the data yet
    return;
  }
  source->doReadFromFile();
}

void LoopByteStreamFileSource::doReadFromFile() {
  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  fFrameSize = fread(fTo, 1, fMaxSize, fFid);
#else
  if (fFidIsSeekable) {
    fFrameSize = fread(fTo, 1, fMaxSize, fFid);
  } else {
    // For non-seekable files (e.g., pipes), call "read()" rather than "fread()", to ensure that the read doesn't block:
    fFrameSize = read(fileno(fFid), fTo, fMaxSize);
  }
#endif
  if (fFrameSize == 0) {
    handleClosure();
    return;
  }
  fNumBytesToStream -= fFrameSize;

  // Set the 'presentation time':
  if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);
    } else {
      // Increment by the play time of the previous data:
      unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
      fPresentationTime.tv_sec += uSeconds/1000000;
      fPresentationTime.tv_usec = uSeconds%1000000;
    }

    // Remember the play time of this data:
    fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
    fDurationInMicroseconds = fLastPlayTime;
  } else {
    // We don't know a specific play time duration for this data,
    // so just record the current time as being the 'presentation time':
    gettimeofday(&fPresentationTime, NULL);
  }

  // Inform the reader that he has data:
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                (TaskFunc*)FramedSource::afterGetting, this);
#else
  // Because the file read was done from the event loop, we can call the
  // 'after getting' function directly, without risk of infinite recursion:
  FramedSource::afterGetting(this);
#endif
}
