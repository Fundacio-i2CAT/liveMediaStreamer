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
  : AudioRTPSink(env, RTPgs, 21, 48000, "UltraGridA"),
    fchannelIDx(1), fBufferIDx(0), fBPS(16), fSample_rate(48000){
}

UltraGridAudioRTPSink::~UltraGridAudioRTPSink() {
}

UltraGridAudioRTPSink*
UltraGridAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new UltraGridAudioRTPSink(env, RTPgs);
}

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

//	uint32_t tmp;
//	tmp = channel << 22; /* bits 0-9 */
//	tmp |= tx->buffer; /* bits 10-31 */
//	audio_hdr[0] = htonl(tmp);
//
//	audio_hdr[2] = htonl(buffer->get_data_len(channel));
//
//	/* fourth word */
//	tmp = (buffer->get_bps() * 8) << 26;
//	tmp |= buffer->get_sample_rate();
//	audio_hdr[3] = htonl(tmp);
//
//	/* fifth word */
//	audio_hdr[4] = htonl(get_audio_tag(buffer->get_codec()));
	/*word 5*/ //AUDIO TAG = 0x7375704F ->	{AC_OPUS, { "OPUS", 0x7375704F }}, // == Opus, the TwoCC isn't defined
//    data = chan_data + pos;
//    data_len = tx->mtu - 40 - sizeof(audio_payload_hdr_t);
//    if(pos + data_len >= (unsigned int) buffer->get_data_len(channel)) {
//            data_len = buffer->get_data_len(channel) - pos;
//            if(channel == buffer->get_channel_count() - 1)
//                    m = 1;
//    }
//    audio_hdr[1] = htonl(pos);
//    pos += data_len;


	// Set the 2-byte "payload header", as defined in RFC 4184.
	unsigned char headers[20];

	Boolean isFragment = numRemainingBytes > 0 || fragmentationOffset > 0;
	if (!isFragment) {
		headers[0] = 0; // One or more complete frames
		headers[1] = 1; // because we (for now) allow at most 1 frame per packet
	} else {
		if (fragmentationOffset > 0) {
			headers[0] = 3; // Fragment of frame other than initial fragment
		} else {
	// An initial fragment of the frame
			unsigned const totalFrameSize = fragmentationOffset
					+ numBytesInFrame + numRemainingBytes;
			unsigned const fiveEighthsPoint = totalFrameSize / 2
					+ totalFrameSize / 8;
			headers[0] = numBytesInFrame >= fiveEighthsPoint ? 1 : 2;

	// Because this outgoing packet will be full (because it's an initial fragment), we can compute how many total
	// fragments (and thus packets) will make up the complete AC-3 frame:
			fTotNumFragmentsUsed = (totalFrameSize + (numBytesInFrame - 1))
					/ numBytesInFrame;
		}

		headers[1] = fTotNumFragmentsUsed;
	}

	setSpecialHeaderBytes(headers, sizeof headers);

	if (numRemainingBytes == 0) {
	// This packet contains the last (or only) fragment of the frame.
	// Set the RTP 'M' ('marker') bit:
		setMarkerBit();
	}

	// Important: Also call our base class's doSpecialFrameHandling(),
	// to set the packet's timestamp:
	MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset, frameStart,
			numBytesInFrame, framePresentationTime, numRemainingBytes);
}

unsigned UltraGridAudioRTPSink::specialHeaderSize() const {
	return 20;
}
