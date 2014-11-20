/*
 *  UltraGridAudioRTPSink.cpp - It consumes audio frames from the frame queue on demand
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

#include "UltraGridAudioRTPSink.hh"
#include <string>
#include <cmath>

#ifdef WORDS_BIGENDIAN
#define to_fourcc(a,b,c,d)     (((uint32_t)(d)) | ((uint32_t)(c)<<8) | ((uint32_t)(b)<<16) | ((uint32_t)(a)<<24))
#else
#define to_fourcc(a,b,c,d)     (((uint32_t)(a)) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))
#endif

UltraGridAudioRTPSink
::UltraGridAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : AudioRTPSink(env, RTPgs, 21, 90000, "UltraGrid"),
		  fWidth(0), fHeight(0),
		  fFPS(25), fInterlacing(0),
		  fTileIDx(0),fBufferIDx(0), fPos(0)) {
}

UltraGridAudioRTPSink::~UltraGridAudioRTPSink() {
}

UltraGridAudioRTPSink*
UltraGridAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new UltraGridAudioRTPSink(env, RTPgs);
}

//TODO Possible check to assure proper MediaSource to UltraGridRTPSink
//Boolean UltraGridAudioRTPSink::sourceIsCompatibleWithUs(MediaSource& source) {
//  return source.isUltraGridAudioSource();
//}

Boolean UltraGridAudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
				 unsigned /*numBytesInFrame*/) const {
  // A packet can contain only one frame
  return False;
}

void UltraGridAudioRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval framePresentationTime,
			 unsigned numRemainingBytes) {

  // the UltraGrid RTP payload header (6 words)
  u_int32_t mainUltraGridHeader[6];
  u_int32_t tmp;
  unsigned int fpsd, fd, fps, fi;

  /* word 4 */
  mainUltraGridHeader[3] = htonl(fWidth << 16 | fHeight);
  /* word 5 */
  mainUltraGridHeader[4] = to_fourcc('A','V','C','1'); //forcing H264 only //get_fourcc(frame->color_spec);
  /* word 3 */
  mainUltraGridHeader[2] = htonl(numBytesInFrame); //frame->tiles[tile_idx].data_len
  tmp = fTileIDx << 22;
  tmp |= 0x3fffff & fBufferIDx;
  /* word 1 */
  mainUltraGridHeader[0] = htonl(tmp);

  /* word 6 */
  tmp = fInterlacing << 29;
  fps = round(fFPS);
  fpsd = 1;
  if(fabs(fFPS - round(fFPS) / 1.001) < 0.005){
		 fd = 1;
  }
  else{
		 fd = 0;
  }
  fi = 0;

  tmp |= fps << 19;
  tmp |= fpsd << 15;
  tmp |= fd << 14;
  tmp |= fi << 13;
  mainUltraGridHeader[5] = htonl(tmp); //format_interl_fps_hdr_row(frame->interlacing, frame->fps);

  /* word 2 */
  if(fPos >= numBytesInFrame){
	  fPos += numRemainingBytes;
  } else {
	  fPos += OutPacketBuffer::maxSize;
  }
  int offset = fPos; // + fragment_offset = 0
  mainUltraGridHeader[1] = htonl(offset);

  //u_int32_t to u_int8_t alignment
  u_int8_t byteAlignedHeader[32];
  for(u_int8_t i=0; i < 6; i++){
	  for(u_int8_t j=0; j < 4; j++){
		  byteAlignedHeader[(i << 2)+j] = (u_int8_t) mainUltraGridHeader[i] >> (24 - (j << 3));
	  }
  }
  // setting the UltraGrid RTP payload header into liveMedia
  setSpecialHeaderBytes(byteAlignedHeader, sizeof byteAlignedHeader);

  if (framerSource != NULL && framerSource->pictureEndMarker() && numRemainingBytes == 0) {
	    // This packet contains the last (or only) fragment of the frame.
	    // Set the RTP 'M' ('marker') bit:
	    setMarkerBit();
        framerSource->pictureEndMarker() = False;
    	fPos = 0;
    	fBufferIDx++;
  }

  // Also set the RTP timestamp:
  setTimestamp(framePresentationTime);
}


unsigned UltraGridAudioRTPSink::specialHeaderSize() const {

  unsigned headerSize = 6; // by default 6 words without FEC payload header

  //TODO for FEC implementations
//  u_int8_t const type = source->type();
//  if (type >= 64 && type <= 127) {
//    // There is also a Restart Marker Header:
//    headerSize += 4;
//  }
//
//  if (curFragmentationOffset() == 0 && source->qFactor() >= 128) {
//    // There is also a Quantization Header:
//    u_int8_t dummy;
//    u_int16_t quantizationTablesSize;
//    (void)(source->quantizationTables(dummy, quantizationTablesSize));
//
//    headerSize += 4 + quantizationTablesSize;
//  }

  return headerSize;
}
