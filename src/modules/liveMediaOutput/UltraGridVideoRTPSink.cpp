/*
 *  UltraGridVideoRTPSink.cpp - It consumes video frames from the frame queue on demand
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

#include "UltraGridVideoRTPSink.hh"
//TODO #include "H264VideoStreamSampler.hh"
#include <string>
#include <cmath>

#ifdef WORDS_BIGENDIAN
#define to_fourcc(a,b,c,d)     (((uint32_t)(d)) | ((uint32_t)(c)<<8) | ((uint32_t)(b)<<16) | ((uint32_t)(a)<<24))
#else
#define to_fourcc(a,b,c,d)     (((uint32_t)(a)) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))
#endif

////////// UltraGridVideoFragmenter definition //////////

// Because of the ideosyncracies of the UltraGrid video RTP payload format, we implement
// "UltraGridVideoRTPSink" using a separate "UltraGridVideoFragmenter" class that delivers,
// to the "UltraGridVideoRTPSink", only fragments that will fit within an outgoing
// RTP packet.  I.e., we implement fragmentation in this separate "UltraGridVideoFragmenter"
// class, rather than in "UltraGridVideoRTPSink".
// (Note: This class should be used only by "UltraGridVideoRTPSink", or a subclass.)

class UltraGridVideoFragmenter: public FramedFilter {
public:
	UltraGridVideoFragmenter(int hNumber, UsageEnvironment& env, FramedSource* inputSource,
		    unsigned inputBufferMax, unsigned maxOutputPacketSize);
  virtual ~UltraGridVideoFragmenter();

  Boolean lastFragmentCompletedFrameUnit() const { return fLastFragmentCompletedFrameUnit; }

private: // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);
public:
  u_int32_t& mainUltraGridHeader() { return fMainUltraGridHeader; }
  int& tileIDx() { return fTileIDx; }
  int& bufferIDx() { return fBufferIDx; }
  unsigned int& pos() { return fPos; }

private:
  int fHNumber;
  unsigned fInputBufferSize;
  unsigned fMaxOutputPacketSize;
  unsigned char* fInputBuffer;
  unsigned fNumValidDataBytes;
  unsigned fCurDataOffset;
  unsigned fSaveNumTruncatedBytes;
  Boolean fLastFragmentCompletedFrameUnit;

protected:
  u_int32_t fMainUltraGridHeader[6];
  int fTileIDx;
  int fBufferIDx;
  unsigned int fPos;
};

////////// UltraGridVideoRTPSink implementation //////////

UltraGridVideoRTPSink::UltraGridVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : VideoRTPSink(env, RTPgs, 20, 90000, "UltraGridV"),
		  fWidth(1920), fHeight(1080),
		  fFPS(25), fInterlacing(0) {
}

UltraGridVideoRTPSink::~UltraGridVideoRTPSink() {
  fSource = fOurFragmenter; // hack: in case "fSource" had gotten set to NULL before we were called
  stopPlaying(); // call this now, because we won't have our 'fragmenter' when the base class destructor calls it later.

  // Close our 'fragmenter' as well:
  fOurFragmenter->mainUltraGridHeader() = NULL;
  Medium::close(fOurFragmenter);
  fSource = NULL; // for the base class destructor, which gets called next
}

UltraGridVideoRTPSink*
UltraGridVideoRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new UltraGridVideoRTPSink(env, RTPgs);
}

Boolean UltraGridVideoRTPSink::continuePlaying() {
  // First, check whether we have a 'fragmenter' class set up yet.
  // If not, create it now:
  u_int32_t fHeaderTmp;
  unsigned int fpsd, fd, fps, fi;
  if (fOurFragmenter == NULL) {
	fOurFragmenter = new UltraGridVideoFragmenter(fHNumber, envir(), fSource, OutPacketBuffer::maxSize,
					   ourMaxPacketSize() - 12/*RTP hdr size*/);
    H264VideoStreamSampler* framerSource
          = (H264VideoStreamSampler*)(fOurFragmenter->inputSource());

    //init UltraGrid video RTP payload header parameters (6 words)
    fOurFragmenter->tileIDx = 0;
    fOurFragmenter->bufferIDx = 0;
    fOurFragmenter->pos = 0;

	/* word 4 */
    fWidth = framerSource->getWidth();
    fHeight = framerSource->getHeight();
    fOurFragmenter->mainUltraGridHeader()[3] = htonl(fWidth << 16 | fHeight);
	/* word 5 */
    fOurFragmenter->mainUltraGridHeader()[4] = to_fourcc('A', 'V', 'C', '1'); //forcing H264 only //get_fourcc(frame->color_spec);

	/* word 6 */
	fHeaderTmp = fInterlacing << 29;
	fps = round(fFPS);
	fpsd = 1;
	if (fabs(fFPS - round(fFPS) / 1.001) < 0.005) {
		fd = 1;
	} else {
		fd = 0;
	}
	fi = 0;

	fHeaderTmp |= fps << 19;
	fHeaderTmp |= fpsd << 15;
	fHeaderTmp |= fd << 14;
	fHeaderTmp |= fi << 13;
	fOurFragmenter->mainUltraGridHeader()[5] = htonl(fHeaderTmp); //format_interl_fps_hdr_row(frame->interlacing, frame->fps);

  } else {
    fOurFragmenter->reassignInputSource(fSource);
  }
  fSource = fOurFragmenter;

  // Then call the parent class's implementation:
  return MultiFramedRTPSink::continuePlaying();
}

void UltraGridVideoRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval framePresentationTime,
			 unsigned numRemainingBytes) {
  // Set the RTP 'M' (marker) bit iff
  // 1/ The most recently delivered fragment was the end of (or the only fragment of) an Frame unit, and
  // 2/ This Frame unit was the last Frame unit of an 'access unit' (i.e. video frame).
  if (fOurFragmenter != NULL) {
	  H264VideoStreamSampler* framerSource
      = (H264VideoStreamSampler*)(fOurFragmenter->inputSource());
    // This relies on our fragmenter's source being a "H264VideoStreamSampler".
    if (((UltraGridVideoFragmenter*)fOurFragmenter)->lastFragmentCompletedFrameUnit()
	&& framerSource != NULL && framerSource->pictureEndMarker()) {
      setMarkerBit();
      framerSource->pictureEndMarker() = False;
    }
  }

  setTimestamp(framePresentationTime);
}

Boolean UltraGridVideoRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
				 unsigned /*numBytesInFrame*/) const {
  // A packet can contain only one frame
  return False;
}

////////// UltraGridVideoFragmenter implementation //////////

UltraGridVideoFragmenter::UltraGridVideoFragmenter(int hNumber,
				     UsageEnvironment& env, FramedSource* inputSource,
				     unsigned inputBufferMax, unsigned maxOutputPacketSize)
  : FramedFilter(env, inputSource),
    fHNumber(hNumber),
    fInputBufferSize(inputBufferMax+1), fMaxOutputPacketSize(maxOutputPacketSize),
    fNumValidDataBytes(1), fCurDataOffset(1), fSaveNumTruncatedBytes(0),
    fLastFragmentCompletedFrameUnit(True) {
  fInputBuffer = new unsigned char[fInputBufferSize];
}

UltraGridVideoFragmenter::~UltraGridVideoFragmenter() {
  delete[] fInputBuffer;
  detachInputSource(); // so that the subsequent ~FramedFilter() doesn't delete it
}

void UltraGridVideoFragmenter::doGetNextFrame() {
  u_int32_t fHeaderTmp;

  if (fNumValidDataBytes == 1) {
    // We have no Frame unit data currently in the buffer.  Read a new one:
    fInputSource->getNextFrame(&fInputBuffer[24], fInputBufferSize - 24,
			       afterGettingFrame, this,
			       FramedSource::handleClosure, this);
  } else {
    // We have Frame unit data in the buffer.  There are three cases to consider:
    // 1. There is a new Frame unit in the buffer, and it's small enough to deliver
    //    to the RTP sink (as is).
    // 2. There is a new Frame unit in the buffer, but it's too large to deliver to
    //    the RTP sink in its entirety.  Deliver the first fragment of this data.
    // 3. There is a Frame unit in the buffer, and we've already delivered some
    //    fragment(s) of this.  Deliver the next fragment of this data.
    if (fMaxSize < fMaxOutputPacketSize) { // shouldn't happen
      envir() << "UltraGridVideoFragmenter::doGetNextFrame(): fMaxSize ("
	      << fMaxSize << ") is smaller than expected\n";
    } else {
      fMaxSize = fMaxOutputPacketSize;
    }

    fLastFragmentCompletedFrameUnit = True; // by default
    if (fCurDataOffset == 1) { // case 1 or 2
      if (fNumValidDataBytes - 24 <= fMaxSize) { // case 1
		//set UltraGrid video RTP payload header (6 words)
    	/* word 3 */
		fMainUltraGridHeader[2] = htonl(fInputBufferSize - 24); //frame->tiles[tile_idx].data_len

		/* word 1 */
		fHeaderTmp = 0 << 22; //fTileIDx = 0
		fHeaderTmp |= 0x3fffff & fBufferIDx;
		fMainUltraGridHeader[0] = htonl(fHeaderTmp);

		/* word 2 */
		int offset = fInputBufferSize - 24;
		fMainUltraGridHeader[1] = htonl(offset);

		//u_int32_t to u_int8_t alignment
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 4; j++) {
				fInputBuffer[(i << 2) + j] = (unsigned char*) fMainUltraGridHeader[i] >> (23 - (j << 3));
			}
		}

		memmove(fTo, fInputBuffer, fNumValidDataBytes);
		fFrameSize = fNumValidDataBytes;
		fCurDataOffset = fNumValidDataBytes;
		fBufferIDx++;
      }

      else { // case 2
		// Deliver the first packet now.
  		//set UltraGrid video RTP payload header (6 words)
      	/* word 3 */
  		fMainUltraGridHeader[2] = htonl(fInputBufferSize - 24); //frame->tiles[tile_idx].data_len

  		/* word 1 */
  		fHeaderTmp = fTileIDx << 22;
  		fHeaderTmp |= 0x3fffff & fBufferIDx;
  		fMainUltraGridHeader[0] = htonl(fHeaderTmp);

  		/* word 2 */
  		int offset = fMaxSize - 24;
  		fMainUltraGridHeader[1] = htonl(offset);

  		//u_int32_t to u_int8_t alignment
  		for (int i = 0; i < 6; i++) {
  			for (int j = 0; j < 4; j++) {
  				fInputBuffer[(i << 2) + j] = (unsigned char*) fMainUltraGridHeader[i] >> (23 - (j << 3));
  			}
  		}

		memmove(fTo, fInputBuffer, fMaxSize);
		fFrameSize = fMaxSize;
		fCurDataOffset += fMaxSize - 24;
		fLastFragmentCompletedFrameUnit = False;
      }
    }
    else { // case 3
		// We've already sent the first packet (fragment).  Now, send the next fragment.
		// Set fLastFragmentCompletedFrameUnit = False if there are more packets to deliver yet.
		unsigned numExtraHeaderBytes = 24;

		unsigned numBytesToSend = numExtraHeaderBytes
				+ (fNumValidDataBytes - fCurDataOffset);
		if (numBytesToSend > fMaxSize) {
			// We can't send all of the remaining data this time:
			numBytesToSend = fMaxSize;
			fLastFragmentCompletedFrameUnit = False;
		} else {
			// This is the last fragment
			fNumTruncatedBytes = fSaveNumTruncatedBytes;
		}

  		//set UltraGrid video RTP payload header (6 words)
  		/* word 2 */
  		int offset = numBytesToSend - 24;
  		fMainUltraGridHeader[1] = htonl(offset);

  		//u_int32_t to u_int8_t alignment
  		for (int i = 0; i < 6; i++) {
  			for (int j = 0; j < 4; j++) {
  				fInputBuffer[(i << 2) + j] = (unsigned char*) fMainUltraGridHeader[i] >> (23 - (j << 3));
  			}
  		}

		memmove(fTo, &fInputBuffer[fCurDataOffset - numExtraHeaderBytes],
				numBytesToSend);
		fFrameSize = numBytesToSend;
		fCurDataOffset += numBytesToSend - numExtraHeaderBytes;
	}

	if (fCurDataOffset >= fNumValidDataBytes) {
		// We're done with this data.  Reset the pointers for receiving new data:
		fNumValidDataBytes = fCurDataOffset = 1;
		fBufferIDx++;
	}

    // Complete delivery to the client:
    FramedSource::afterGetting(this);
  }
}

void UltraGridVideoFragmenter::afterGettingFrame(void* clientData, unsigned frameSize,
					  unsigned numTruncatedBytes,
					  struct timeval presentationTime,
					  unsigned durationInMicroseconds) {
  UltraGridVideoFragmenter* fragmenter = (UltraGridVideoFragmenter*)clientData;
  fragmenter->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime,
				 durationInMicroseconds);
}

void UltraGridVideoFragmenter::afterGettingFrame1(unsigned frameSize,
					   unsigned numTruncatedBytes,
					   struct timeval presentationTime,
					   unsigned durationInMicroseconds) {
  fNumValidDataBytes += frameSize;
  fSaveNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;

  // Deliver data to the client:
  doGetNextFrame();
}
