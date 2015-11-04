/*
 *  LoopByteStreamFileSource.hh - Class that loops the input file.
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *
 */

#ifndef _LOOP_BYTE_STREAM_FILE_SOURCE_HH
#define _LOOP_BYTE_STREAM_FILE_SOURCE_HH

#include <liveMedia.hh>

class LoopByteStreamFileSource: public FramedFileSource {
public:
  static LoopByteStreamFileSource* createNew(UsageEnvironment& env,
                                         char const* fileName, int& reset,
                                         unsigned preferredFrameSize = 0,
                                         unsigned playTimePerFrame = 0);


  static LoopByteStreamFileSource* createNew(UsageEnvironment& env,
                                         FILE* fid, int& reset,
                                         unsigned preferredFrameSize = 0,
                                         unsigned playTimePerFrame = 0);

  u_int64_t fileSize() const { return fFileSize; }

  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
  void seekToByteRelative(int64_t offset, u_int64_t numBytesToStream = 0);
  void seekToEnd(); 

protected:
  LoopByteStreamFileSource(UsageEnvironment& env,
                       FILE* fid, int& reset,
                       unsigned preferredFrameSize,
                       unsigned playTimePerFrame);

  virtual ~LoopByteStreamFileSource();

  static void fileReadableHandler(LoopByteStreamFileSource* source, int mask);
  void doReadFromFile();

private:

  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

protected:
  u_int64_t fFileSize;

private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream;
  int fReset;
};

#endif