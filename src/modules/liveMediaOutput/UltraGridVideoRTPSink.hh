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

class UltraGridVideoRTPSink: public VideoRTPSink {
public:
  static UltraGridVideoRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned int width = 0, unsigned int height = 0,
		  double fps = 25, int interlacing = 0,
		  int tileIDx = 0, int bufferIDx = 0, unsigned int pos = 0);

protected:
  UltraGridVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned int width, unsigned int height,
		  double fps, int interlacing,
		  int tileIDx, int bufferIDx, unsigned int pos);
	// called only by createNew()

  virtual ~UltraGridVideoRTPSink();

  //internal variables for payload header info
  double fFPS;
  unsigned int fWidth;
  unsigned int fHeight;
  int fInterlacing; /*	PROGRESSIVE       = 0, ///< progressive frame
        				UPPER_FIELD_FIRST = 1, ///< First stored field is top, followed by bottom
        				LOWER_FIELD_FIRST = 2, ///< First stored field is bottom, followed by top
        				INTERLACED_MERGED = 3, ///< Columngs of both fields are interlaced together
        				SEGMENTED_FRAME   = 4,  ///< Segmented frame. Contains the same data as progressive frame.*/
  int fTileIDx;
  int fBufferIDx;
  unsigned int fPos;
//  int fHdrs_len = 40 + 8 + 12; // IPv6 hdr size + UDP hdr size + RTP hdr size
//  int fData_len = /*MTU*/ fOurMaxPacketSize - fHdrs_len;

private: // redefined virtual functions:
  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);

  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
  virtual unsigned specialHeaderSize() const;
};

#endif
