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
#include "H264VideoStreamSampler.hh"
#include <cmath>

#ifdef WORDS_BIGENDIAN
#define to_fourcc(a,b,c,d)     (((uint32_t)(d)) | ((uint32_t)(c)<<8) | ((uint32_t)(b)<<16) | ((uint32_t)(a)<<24))
#else
#define to_fourcc(a,b,c,d)     (((uint32_t)(a)) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))
#endif

#define UG_PAYLOAD_HEADER_SIZE 24
////////// UltraGridVideoFragmenter definition //////////

// Because of the ideosyncracies of the UltraGrid video RTP payload format, we implement
// "UltraGridVideoRTPSink" using a separate "UltraGridVideoFragmenter" class that delivers,
// to the "UltraGridVideoRTPSink", only fragments that will fit within an outgoing
// RTP packet.  I.e., we implement fragmentation in this separate "UltraGridVideoFragmenter"
// class, rather than in "UltraGridVideoRTPSink".
// (Note: This class should be used only by "UltraGridVideoRTPSink", or a subclass.)

class UltraGridVideoFragmenter: public FramedFilter {
public:
	UltraGridVideoFragmenter(UsageEnvironment& env, FramedSource* inputSource,
		    unsigned inputBufferMax, unsigned maxOutputPacketSize);
  virtual ~UltraGridVideoFragmenter();

  Boolean lastFragmentCompletedFrameUnit() const { return fLastFragmentCompletedFrameUnit; }

  void setWidth(int width);
  void setHeight(int height);

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

private:
  unsigned fInputBufferSize;
  unsigned fMaxOutputPacketSize;
  unsigned char* fInputBuffer;
  unsigned fNumValidDataBytes;
  unsigned fCurDataOffset;
  unsigned fSaveNumTruncatedBytes;
  Boolean fLastFragmentCompletedFrameUnit;

protected:
  uint32_t fMainUltraGridHeader[6];
  int fTileIDx;
  int fBufferIDx;
  unsigned int fWidth;
  unsigned int fHeight;
  double fFPS;
  int fInterlacing; /*	PROGRESSIVE       = 0, ///< progressive frame
        				UPPER_FIELD_FIRST = 1, ///< First stored field is top, followed by bottom
        				LOWER_FIELD_FIRST = 2, ///< First stored field is bottom, followed by top
        				INTERLACED_MERGED = 3, ///< Columngs of both fields are interlaced together
        				SEGMENTED_FRAME   = 4,  ///< Segmented frame. Contains the same data as progressive frame.*/
};

////////// UltraGridVideoRTPSink implementation //////////

UltraGridVideoRTPSink::UltraGridVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : VideoRTPSink(env, RTPgs, 20, 90000, "UltraGridV"), validVideoSize(False) {
    fDummyBuf = new unsigned char[OutPacketBuffer::maxSize];
}

UltraGridVideoRTPSink::~UltraGridVideoRTPSink() {
  fSource = fOurFragmenter; // hack: in case "fSource" had gotten set to NULL before we were called
  stopPlaying(); // call this now, because we won't have our 'fragmenter' when the base class destructor calls it later.

  delete[] fDummyBuf;
  // Close our 'fragmenter' as well:
  Medium::close(fOurFragmenter);
  fSource = NULL; // for the base class destructor, which gets called next
}

UltraGridVideoRTPSink*
UltraGridVideoRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new UltraGridVideoRTPSink(env, RTPgs);
}

Boolean UltraGridVideoRTPSink::continuePlaying()
{
    if (!validVideoSize){
        return continuePlayingDummy();
    }
  
    if (fOurFragmenter == NULL) {
        fOurFragmenter = new UltraGridVideoFragmenter(envir(), fSource, OutPacketBuffer::maxSize,
                                                        ourMaxPacketSize() - 12/*RTP hdr size*/);

        ((UltraGridVideoFragmenter*)fOurFragmenter)->setWidth(((H264VideoStreamSampler*) fSource)->getWidth());
        ((UltraGridVideoFragmenter*)fOurFragmenter)->setHeight(((H264VideoStreamSampler*) fSource)->getHeight());
    } else {
        fOurFragmenter->reassignInputSource(fSource);
    }
  
    fSource = fOurFragmenter;
  
    return MultiFramedRTPSink::continuePlaying();
}

Boolean UltraGridVideoRTPSink::continuePlayingDummy()
{
	fSource->getNextFrame(fDummyBuf, OutPacketBuffer::maxSize,
                          afterGettingFrameDummy, this, FramedSource::handleClosure, this);
	return True;
}

void UltraGridVideoRTPSink
::afterGettingFrameDummy(void* clientData, unsigned numBytesRead,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  UltraGridVideoRTPSink* sink = (UltraGridVideoRTPSink*)clientData;
  sink->afterGettingFrameDummy1(numBytesRead, numTruncatedBytes,
                           presentationTime, durationInMicroseconds);
}

void UltraGridVideoRTPSink
::afterGettingFrameDummy1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds)
{
    H264VideoStreamSampler* sampler = (H264VideoStreamSampler*) fSource;

    if (sampler->getWidth() <= 0 || sampler->getHeight() <= 0){
        fSource->getNextFrame(fDummyBuf, OutPacketBuffer::maxSize,
                          afterGettingFrameDummy, this, FramedSource::handleClosure, this);
    } else {
        validVideoSize = True;
        continuePlaying();
    }
}

// void UltraGridVideoRTPSink::ourHandleClosure(void* clientData) {
//   UltraGridVideoRTPSink* sink = (UltraGridVideoRTPSink*)clientData;
//   // There are no frames left, but we may have a partially built packet
//   //  to send
//   sink->fNoFramesLeft = True;
//   sink->sendPacketIfNecessary();
// }

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

UltraGridVideoFragmenter::UltraGridVideoFragmenter(UsageEnvironment& env,
						FramedSource* inputSource, unsigned inputBufferMax,
						unsigned maxOutputPacketSize)
  : FramedFilter(env, inputSource),
    fInputBufferSize(inputBufferMax), fMaxOutputPacketSize(maxOutputPacketSize),
    fNumValidDataBytes(0), fCurDataOffset(0), fSaveNumTruncatedBytes(0),
    fLastFragmentCompletedFrameUnit(True), fTileIDx(0), fBufferIDx(0),
    fWidth(0), fHeight(0), fFPS(25), fInterlacing(0) {
  fInputBuffer = new unsigned char[fInputBufferSize];
}

UltraGridVideoFragmenter::~UltraGridVideoFragmenter() {
  delete[] fInputBuffer;
  detachInputSource(); // so that the subsequent ~FramedFilter() doesn't delete it
}

void UltraGridVideoFragmenter::doGetNextFrame() {
  uint32_t fHeaderTmp;
  unsigned int fpsd, fd, fps, fi;
  unsigned numBytesToSend = 0;

  if (fNumValidDataBytes == 0) {
    // We have no Frame unit data currently in the buffer.  Read a new one:
    fInputSource->getNextFrame(fInputBuffer, fInputBufferSize,
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
    if (fCurDataOffset == 0) { // case 1 or 2

		//TODO dynamic get width, height and FPS from frameSource
		//		H264VideoStreamSampler* framerSource = (H264VideoStreamSampler*)(fOurFragmenter->inputSource());
		//
		//		//UltraGrid video RTP payload header (6 words) and parameters
		//		/* word 4 */
		//		fWidth = framerSource->getWidth();
		//		fHeight = framerSource->getHeight();
		//		fFPS = framerSource->getFPS();
		fMainUltraGridHeader[3] = (fWidth << 16 | fHeight);

		/* word 5 */
		fMainUltraGridHeader[4] = htonl(to_fourcc('A', 'V', 'C', '1'));

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
		fMainUltraGridHeader[5] = fHeaderTmp;
		fHeaderTmp = 0;

		if (fNumValidDataBytes + UG_PAYLOAD_HEADER_SIZE <= fMaxSize) { // case 1
		  numBytesToSend = fNumValidDataBytes + UG_PAYLOAD_HEADER_SIZE;
		}
		else {
	  	  numBytesToSend = fMaxSize;
		  fLastFragmentCompletedFrameUnit = False;
		}

		//set UltraGrid video RTP payload header (6 words)
    	/* word 3 */
		fMainUltraGridHeader[2] = fNumValidDataBytes;

		/* word 1 */
		fHeaderTmp = fTileIDx << 22;
		fHeaderTmp |= 0x3fffff & fBufferIDx;
		fMainUltraGridHeader[0] = fHeaderTmp;

		/* word 2 */
		fMainUltraGridHeader[1] = 0;

		//uint32_t to uint8_t alignment
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 4; j++) {
				fTo[(i << 2) + j] = (unsigned char) (fMainUltraGridHeader[i] >> (UG_PAYLOAD_HEADER_SIZE - (j << 3)));
			}
		}

		memmove(fTo + UG_PAYLOAD_HEADER_SIZE, fInputBuffer, fNumValidDataBytes + UG_PAYLOAD_HEADER_SIZE);

		fFrameSize = numBytesToSend;
		fCurDataOffset += numBytesToSend -UG_PAYLOAD_HEADER_SIZE;
    }
    else { // case 3
		// We've already sent the first packet (fragment).  Now, send the next fragment.
		// Set fLastFragmentCompletedFrameUnit = False if there are more packets to deliver yet.
		numBytesToSend = UG_PAYLOAD_HEADER_SIZE
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
  		fMainUltraGridHeader[1] = fCurDataOffset;
  		//uint32_t to uint8_t alignment
 		for (int i = 0; i < 6; i++) {
  			for (int j = 0; j < 4; j++) {
  				fTo[(i << 2) + j] = (unsigned char) (fMainUltraGridHeader[i] >> (UG_PAYLOAD_HEADER_SIZE - (j << 3)));
			}
  		}

		memmove(fTo + UG_PAYLOAD_HEADER_SIZE, &fInputBuffer[fCurDataOffset], numBytesToSend - UG_PAYLOAD_HEADER_SIZE);

		fFrameSize = numBytesToSend;
		fCurDataOffset += numBytesToSend - UG_PAYLOAD_HEADER_SIZE;
	}

	if (fCurDataOffset == fNumValidDataBytes) {
		// We're done with this data.  Reset the pointers for receiving new data:
		fNumValidDataBytes = 0;
		fCurDataOffset = 0;
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
  fNumValidDataBytes = frameSize;
  fSaveNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;

  // Deliver data to the client:
  doGetNextFrame();
}

void UltraGridVideoFragmenter::setWidth(int width){
	fWidth = width;
}

void UltraGridVideoFragmenter::setHeight(int height){
	fHeight = height;
}
