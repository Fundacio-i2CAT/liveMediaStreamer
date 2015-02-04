/*
 *  UltraGridVideoRTPSink.hh - It consumes video frames from the frame queue on demand
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
 *  Authors:  Gerard Castillo <gerard.castillo@i2cat.net>,
 *
 */

#ifndef _UltraGrid_VIDEO_RTP_SINK_HH
#define _UltraGrid_VIDEO_RTP_SINK_HH

#ifndef _VIDEO_RTP_SINK_HH
#include "VideoRTPSink.hh"
#endif
#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

#define UG_PAYLOAD_HEADER_SIZE 24
#define UG_FRAME_MAX_SIZE 1920*1080*3

class UltraGridVideoRTPSink: public VideoRTPSink {
public:
  static UltraGridVideoRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs);

protected:
  UltraGridVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs);
	// called only by createNew()

  virtual ~UltraGridVideoRTPSink();

private: // redefined virtual functions:
  virtual Boolean continuePlaying();
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
                                         
    Boolean continuePlayingDummy();
    
    static void afterGettingFrameDummy(void* clientData, unsigned numBytesRead,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
    
    void afterGettingFrameDummy1(unsigned frameSize, unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
    
    //void ourHandleClosure(void* clientData);

protected:
  //internal variables for payload header info
  FramedFilter* fOurFragmenter;
  Boolean validVideoSize;
  unsigned char* fDummyBuf;
};

#endif
