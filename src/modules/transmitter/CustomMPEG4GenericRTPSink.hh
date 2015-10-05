/*
 *  CustomMPEG4GenericRTPSink.cpp - Modified MPEG4GenericRTPSink
 *  Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors: Marc Palau <marc.palau@i2cat.net>
 *            
 */

#ifndef _CUSTOM_MPEG4_GENERIC_RTP_SINK_HH
#define _CUSTOM_MPEG4_GENERIC_RTP_SINK_HH

#include "MultiFramedRTPSink.hh"

/*! An RTP Sink which consumes AAC ADTS frames. It must be associated to a ADTSStreamParser 
    in order to construct the specific SDP line */

class CustomMPEG4GenericRTPSink: public MultiFramedRTPSink {

public:
    /**
    * Constructor wrapper
    * @param env Live555 environement
    * @param RTPgs Live555 RTP groupsock
    * @param rtpPayloadFormat RTP payload format
    * @param rtpTimestampFrequency RTP timestamp frequency in Hz
    * @param sdpMediaTypeString Only "audio" is supported
    * @param mpeg4Mode Input source, which contains AAC frames
    * @param numChannels Input source, which contains AAC frames
    * @return Pointer to the object if succeded and NULL if not
    */
    static CustomMPEG4GenericRTPSink* 
    createNew(UsageEnvironment& env, Groupsock* RTPgs,
        u_int8_t rtpPayloadFormat, u_int32_t rtpTimestampFrequency,
        char const* sdpMediaTypeString, char const* mpeg4Mode, unsigned numChannels);

protected:
    CustomMPEG4GenericRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
              u_int8_t rtpPayloadFormat,
              u_int32_t rtpTimestampFrequency,
              char const* sdpMediaTypeString,
              char const* mpeg4Mode, unsigned numChannels);

    virtual ~CustomMPEG4GenericRTPSink();

private:
    Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
                     unsigned numBytesInFrame) const;
    void doSpecialFrameHandling(unsigned fragmentationOffset,
                                unsigned char* frameStart,
                                unsigned numBytesInFrame,
                                struct timeval framePresentationTime,
                                unsigned numRemainingBytes);
    unsigned specialHeaderSize() const;
    char const* sdpMediaType() const;
    char const* auxSDPLine(); 

private:
  char const* fSDPMediaTypeString;
  char const* fMPEG4Mode;
  char const* fConfigString;
  char* fFmtpSDPLine;
};

#endif
