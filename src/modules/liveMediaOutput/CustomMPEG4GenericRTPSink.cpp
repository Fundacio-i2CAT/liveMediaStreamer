/*
 *  CustomMPEG4GenericRTPSink.hh - Modified MPEG4GenericRTPSink
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

#include "CustomMPEG4GenericRTPSink.hh"
#include "ADTSStreamParser.hh"
#include "../../Utils.hh"
#include "Locale.hh"
#include <ctype.h> 
#include <iostream> 

CustomMPEG4GenericRTPSink
::CustomMPEG4GenericRTPSink(UsageEnvironment& env, Groupsock* RTPgs, u_int8_t rtpPayloadFormat,
                             u_int32_t rtpTimestampFrequency, char const* sdpMediaTypeString,
                              char const* mpeg4Mode, unsigned numChannels)
: MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency, "MPEG4-GENERIC", numChannels),
  fSDPMediaTypeString(strDup(sdpMediaTypeString)), fMPEG4Mode(strDup(mpeg4Mode)), fConfigString(NULL) 
{

  if (mpeg4Mode == NULL) {
      env << "CustomMPEG4GenericRTPSink error: NULL \"mpeg4Mode\" parameter\n";
  
  } else {
      size_t const len = strlen(mpeg4Mode) + 1;
      char* m = new char[len];

      Locale l("POSIX");

      for (size_t i = 0; i < len; ++i) {
          m[i] = tolower(mpeg4Mode[i]);
      } 

      if (strcmp(m, "aac-hbr") != 0) {
          env << "CustomMPEG4GenericRTPSink error: Unknown \"mpeg4Mode\" parameter: \"" << mpeg4Mode << "\"\n";
      }
      
      delete[] m;
  }

}

CustomMPEG4GenericRTPSink::~CustomMPEG4GenericRTPSink() 
{

}

CustomMPEG4GenericRTPSink*
CustomMPEG4GenericRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs, u_int8_t rtpPayloadFormat,
                                      u_int32_t rtpTimestampFrequency, char const* sdpMediaTypeString,
                                       char const* mpeg4Mode, unsigned numChannels) 
{
    return new CustomMPEG4GenericRTPSink(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency,
                                          sdpMediaTypeString, mpeg4Mode, numChannels);
}

Boolean CustomMPEG4GenericRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
                                 unsigned /*numBytesInFrame*/) const {
  // (For now) allow at most 1 frame in a single packet:
  return False;
}

void CustomMPEG4GenericRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
             unsigned char* frameStart,
             unsigned numBytesInFrame,
             struct timeval framePresentationTime,
             unsigned numRemainingBytes) 
{
  unsigned fullFrameSize = fragmentationOffset + numBytesInFrame + numRemainingBytes;
  unsigned char headers[4];
  headers[0] = 0; headers[1] = 16 /* bits */; // AU-headers-length
  headers[2] = fullFrameSize >> 5; headers[3] = (fullFrameSize&0x1F)<<3;

  setSpecialHeaderBytes(headers, sizeof headers);

  if (numRemainingBytes == 0) {
      setMarkerBit();
  }

  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
                         frameStart, numBytesInFrame,
                         framePresentationTime,
                         numRemainingBytes);
}

unsigned CustomMPEG4GenericRTPSink::specialHeaderSize() const 
{
  return 2 + 2;
}

char const* CustomMPEG4GenericRTPSink::sdpMediaType() const 
{
  return fSDPMediaTypeString;
}

char const* CustomMPEG4GenericRTPSink::auxSDPLine() 
{
    ADTSStreamParser* adts;

    adts = dynamic_cast<ADTSStreamParser*>(fSource);

    if (!adts) {
      utils::errorMsg("Its is not and ADTSStreamParser!");
      return NULL;
    }

    fConfigString = adts->getConfigString();

    if (!fConfigString) {
        return NULL;
    }

    // Set up the "a=fmtp:" SDP line for this stream:
    char const* fmtpFmt =
        "a=fmtp:%d "
        "streamtype=%d;profile-level-id=1;"
        "mode=%s;sizelength=13;indexlength=3;indexdeltalength=3;"
        "config=%s\r\n";

    unsigned fmtpFmtSize = strlen(fmtpFmt)
        + 3 /* max char len */
        + 3 /* max char len */
        + strlen(fMPEG4Mode)
        + strlen(fConfigString);
  
    char* fmtp = new char[fmtpFmtSize];
  
    sprintf(fmtp, fmtpFmt,
        rtpPayloadType(),
        strcmp(fSDPMediaTypeString, "video") == 0 ? 4 : 5,
        fMPEG4Mode,
        fConfigString);
  
    fFmtpSDPLine = strDup(fmtp);
    delete[] fmtp;
    return fFmtpSDPLine;
}