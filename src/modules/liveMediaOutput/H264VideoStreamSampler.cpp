/*
 *  CustomH264or5VideoDiscreteFramer.cpp - Custom H264or5VideoDiscreteFramer liveMedia class.
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

 #include "H264VideoStreamSampler.hh"

H264VideoStreamSampler* H264VideoStreamSampler::createNew(UsageEnvironment& env, FramedSource* inputSource, bool annexB)
{
    return new H264VideoStreamSampler(env, inputSource, annexB);
}

 H264VideoStreamSampler
::H264VideoStreamSampler(UsageEnvironment& env, FramedSource* inputSource, bool annexB)
  : H264or5VideoStreamFramer(264, env, inputSource, False/*don't create a parser*/, False),
  offset(0), totalFrameSize(0), fAnnexB(annexB) {
}

H264VideoStreamSampler::~H264VideoStreamSampler() {
}

void H264VideoStreamSampler::doGetNextFrame() {
    fInputSource->getNextFrame(fTo + offset + NAL_START_SIZE, fMaxSize,
                               afterGettingFrame, this, FramedSource::handleClosure, this);
}

void H264VideoStreamSampler
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  H264VideoStreamSampler* source = (H264VideoStreamSampler*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264VideoStreamSampler
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    unsigned char* realPtr = fTo + offset;
    unsigned char* NALstartPtr = realPtr + NAL_START_SIZE;

    if (fAnnexB) {
        realPtr[0] = 0;
        realPtr[1] = 0;
        realPtr[2] = 0;
        realPtr[3] = 1;
    } else {
        uint32_t nalSizeHtoN = htonl(frameSize);
        memcpy(realPtr, &nalSizeHtoN, NAL_START_SIZE);
    }


    u_int8_t nal_unit_type;
    if (frameSize >= 1) {
        nal_unit_type = NALstartPtr[0]&0x1F;
    } else {
        // This is too short to be a valid NAL unit, so just assume a bogus nal_unit_type
        nal_unit_type = 0xFF;
    }

    if (frameSize >= 4 && NALstartPtr[0] == 0 && NALstartPtr[1] == 0 && ((NALstartPtr[2] == 0 && NALstartPtr[3] == 1) || NALstartPtr[2] == 1)) {
        envir() << "H264VideoStreamSampler error: MPEG 'start code' seen in the input\n";
    } else if (isSPS(nal_unit_type)) { // Sequence parameter set (SPS)
        saveCopyOfSPS(NALstartPtr, frameSize);
        updateVideoSize(NALstartPtr, frameSize);
    } else if (isPPS(nal_unit_type)) { // Picture parameter set (PPS)
        saveCopyOfPPS(NALstartPtr, frameSize);
    }

    if (isVCL(nal_unit_type)) {
        fPictureEndMarker = True;
        fFrameSize = totalFrameSize;
        fNumTruncatedBytes = numTruncatedBytes;
        fPresentationTime = presentationTime;
        fDurationInMicroseconds = durationInMicroseconds;

        resetInternalValues();
        afterGetting(this);
    }

    offset += frameSize + NAL_START_SIZE;
    totalFrameSize += offset;
    doGetNextFrame();
}

void H264VideoStreamSampler::resetInternalValues()
{
    offset = 0;
    totalFrameSize = 0;
}

void H264VideoStreamSampler::updateVideoSize(unsigned char* NALstartPtr, unsigned frameSize)
{

}

int H264VideoStreamSampler::getWidth(){
	return fWidth;
}
void H264VideoStreamSampler::setWidth(int width){

}
int H264VideoStreamSampler::getHeight(){
	return fHeight;
}
void H264VideoStreamSampler::setHeight(int height){

}
