/*
 *  H264or5StartCodeInjector.cpp - H264 NAL Unit start code injector.
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#include "H264or5StartCodeInjector.hh"
#include <iostream>

H264or5StartCodeInjector* H264or5StartCodeInjector::createNew(UsageEnvironment& env, FramedSource* inputSource, VCodecType codec)
{
    int hNumber = 0;
    H264or5StartCodeInjector* injector;

    if (codec == H264) {
        hNumber = 264;
    } else if (codec == H265) {
        hNumber = 265;
    }

    if (hNumber == 0) {
        injector = NULL;
    } else {
        injector = new H264or5StartCodeInjector(env, inputSource, hNumber);
    }

    return injector;
}

 H264or5StartCodeInjector
::H264or5StartCodeInjector(UsageEnvironment& env, FramedSource* inputSource, int hNumber)
  : H264or5VideoStreamFramer(hNumber, env, inputSource, False/*don't create a parser*/, False)
{
}

H264or5StartCodeInjector::~H264or5StartCodeInjector() {
}

void H264or5StartCodeInjector::doGetNextFrame() {
    fInputSource->getNextFrame(fTo + NAL_START_SIZE, fMaxSize - NAL_START_SIZE,
                               afterGettingFrame, this, FramedSource::handleClosure, this);
}

void H264or5StartCodeInjector
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  H264or5StartCodeInjector* source = (H264or5StartCodeInjector*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264or5StartCodeInjector
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    u_int8_t nal_unit_type;
    unsigned char* NALstartPtr;

    NALstartPtr = fTo + NAL_START_SIZE;

    fTo[0] = 0;
    fTo[1] = 0;
    fTo[2] = 0;
    fTo[3] = 1;

    if (fHNumber == 264 && frameSize >= 1) {
        nal_unit_type = NALstartPtr[0]&0x1F;
    } else if (fHNumber == 265 && frameSize >= 2) {
        nal_unit_type = (NALstartPtr[0]&0x7E)>>1;
    } else {
    // This is too short to be a valid NAL unit, so just assume a bogus nal_unit_type
        nal_unit_type = 0xFF;
    }

    if (isVCL(nal_unit_type)) {
        fPictureEndMarker = True;
    }

    // Finally, complete delivery to the client:
    fFrameSize = frameSize + NAL_START_SIZE;
    fNumTruncatedBytes = numTruncatedBytes;
    fPresentationTime = presentationTime;
    fDurationInMicroseconds = durationInMicroseconds;
    afterGetting(this);
}
