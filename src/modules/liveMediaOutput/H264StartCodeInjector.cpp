/*
 *  H264StartCodeInjector.cpp - H264 NAL Unit start code injector.
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

 #include "H264StartCodeInjector.hh"

H264StartCodeInjector* H264StartCodeInjector::createNew(UsageEnvironment& env, FramedSource* inputSource)
{
    return new H264StartCodeInjector(env, inputSource);
}

 H264StartCodeInjector
::H264StartCodeInjector(UsageEnvironment& env, FramedSource* inputSource)
  : H264or5VideoStreamFramer(264, env, inputSource, False/*don't create a parser*/, False)
{
}

H264StartCodeInjector::~H264StartCodeInjector() {
}

void H264StartCodeInjector::doGetNextFrame() {
    fInputSource->getNextFrame(fTo + NAL_START_SIZE, fMaxSize - NAL_START_SIZE,
                               afterGettingFrame, this, FramedSource::handleClosure, this);
}

void H264StartCodeInjector
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  H264StartCodeInjector* source = (H264StartCodeInjector*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264StartCodeInjector
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    unsigned char* NALstartPtr = fTo + NAL_START_SIZE;

    fTo[0] = 0;
    fTo[1] = 0;
    fTo[2] = 0;
    fTo[3] = 1;

    u_int8_t nal_unit_type;
    if (frameSize >= 1) {
        nal_unit_type = NALstartPtr[0]&0x1F;
    } else {
        // This is too short to be a valid NAL unit, so just assume a bogus nal_unit_type
        nal_unit_type = 0xFF;
    }

    if (frameSize >= 4 && NALstartPtr[0] == 0 && NALstartPtr[1] == 0 && ((NALstartPtr[2] == 0 && NALstartPtr[3] == 1) || NALstartPtr[2] == 1)) {
        envir() << "H264StartCodeInjector error: MPEG 'start code' seen in the input\n";
    } else if (isSPS(nal_unit_type)) { // Sequence parameter set (SPS)
        saveCopyOfSPS(NALstartPtr, frameSize);
    } else if (isPPS(nal_unit_type)) { // Picture parameter set (PPS)
        saveCopyOfPPS(NALstartPtr, frameSize);
    }

    if (isVCL(nal_unit_type)) fPictureEndMarker = True;

    // Finally, complete delivery to the client:
    fFrameSize = frameSize + NAL_START_SIZE;
    fNumTruncatedBytes = numTruncatedBytes;
    fPresentationTime = presentationTime;
    fDurationInMicroseconds = durationInMicroseconds;
    afterGetting(this);
}
