/*
 *  UltraGridAudioRTPSink.hh - It consumes audio frames from the frame queue on demand
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

#ifndef _UltraGrid_AUDIO_RTP_SINK_HH
#define _UltraGrid_AUDIO_RTP_SINK_HH

#ifndef _AUDIO_RTP_SINK_HH
#include "AudioRTPSink.hh"
#endif
#include "Types.hh"

class UltraGridAudioRTPSink: public AudioRTPSink {
public:
  static UltraGridAudioRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs, ACodecType codec,
                  unsigned channels, unsigned sampleRate, SampleFmt sampleFormat);

protected:
  UltraGridAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs, ACodecType codec,
                  unsigned channels, unsigned sampleRate, SampleFmt sampleFormat);
	// called only by createNew()

  virtual ~UltraGridAudioRTPSink();

  //internal variables for payload header info
  uint32_t fAudio_tag;
  int fChannels;
  int fchannelIDx;
  int fBufferIDx;
  int fBPS;
  int fSample_rate;

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
