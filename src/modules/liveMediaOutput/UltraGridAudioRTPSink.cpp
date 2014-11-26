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
#include <assert.h>
#include <iostream>
#include <string>
#include <cmath>

#ifdef WORDS_BIGENDIAN
#define to_fourcc(a,b,c,d)     (((uint32_t)(d)) | ((uint32_t)(c)<<8) | ((uint32_t)(b)<<16) | ((uint32_t)(a)<<24))
#else
#define to_fourcc(a,b,c,d)     (((uint32_t)(a)) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))
#endif

#include <unordered_map>
static std::unordered_map<ACodecType, uint32_t, std::hash<int>> LMS_to_UG_mapping = {
        { PCM, 0x0001 },
        { PCMU, 0x0007},
        { OPUS, 0x7375704F },
};

UltraGridAudioRTPSink
::UltraGridAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs, ACodecType codec,
                unsigned channels, unsigned sampleRate, SampleFmt sampleFormat)
  : AudioRTPSink(env, RTPgs, 21, 90000, "UltraGridA"), fChannels(1),
    fchannelIDx(0), fBufferIDx(0), fSample_rate(sampleRate) {
        switch (sampleFormat) {
        case U8:
        case U8P:
                fBPS = 8;
                break;
        case S16:
        case S16P:
                fBPS = 16;
                break;
        default:
                fBPS = 0;
        }
        auto it = LMS_to_UG_mapping.find(codec);
        if (it != LMS_to_UG_mapping.end()) {
                fAudio_tag = it->second;
        } else {
                fAudio_tag = 0;
        }
}

UltraGridAudioRTPSink::~UltraGridAudioRTPSink() {
}

UltraGridAudioRTPSink*
UltraGridAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs, ACodecType codec,
                unsigned channels, unsigned sampleRate, SampleFmt sampleFormat) {
  return new UltraGridAudioRTPSink(env, RTPgs, codec, channels, sampleRate, sampleFormat);
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
        uint32_t audio_hdr[5];
        uint32_t tmp;

        tmp = fchannelIDx << 22; /* bits 0-9 */
        tmp |= fBufferIDx; /* bits 10-31 */
        audio_hdr[0] = htonl(tmp);

        audio_hdr[2] = htonl(numBytesInFrame);
        //std::cout << numBytesInFrame << std::endl;

        /* fourth word */
        tmp = fBPS << 26;
        tmp |= fSample_rate;
        audio_hdr[3] = htonl(tmp);

        /* fifth word */
        audio_hdr[4] = htonl(fAudio_tag);
	/*word 5*/ //AUDIO TAG = 0x7375704F ->	{AC_OPUS, { "OPUS", 0x7375704F }}, // == Opus, the TwoCC isn't defined
        audio_hdr[1] = htonl(fragmentationOffset);
        assert(fragmentationOffset == 0); // not tested

        setSpecialHeaderBytes((unsigned char *) audio_hdr, sizeof audio_hdr);

	if (numRemainingBytes == 0) {
	// This packet contains the last (or only) fragment of the frame.
	// Set the RTP 'M' ('marker') bit:
		setMarkerBit();
                fBufferIDx = (fBufferIDx + 1) & 0x3fffff;
	}

        if (fAudio_tag == 0x1) {
                for (unsigned int i = 0; i < numBytesInFrame; i+=2) {
                        unsigned char sample1 = frameStart[0];
                        unsigned char sample2 = frameStart[1];
                        frameStart[0] = sample2;
                        frameStart[1] = sample1;
                        frameStart += 2;
                }
        }

	// Important: Also call our base class's doSpecialFrameHandling(),
	// to set the packet's timestamp:
        MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset, frameStart,
			numBytesInFrame, framePresentationTime, numRemainingBytes);
}

unsigned UltraGridAudioRTPSink::specialHeaderSize() const {
	return 20;
}

Boolean UltraGridAudioRTPSink::sourceIsCompatibleWithUs(MediaSource& source)
{
        // TODO: should here be something else?
        if (fAudio_tag != 0 && fChannels == 1)
                return True;
        else
                return False;
}

