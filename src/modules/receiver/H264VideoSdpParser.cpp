/*
 *  H264VideoSdpParser.cpp - H264 SDP parser
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

#include "H264VideoSdpParser.hh"
#include "../../Utils.hh"

#include <iostream>

static unsigned char const startCode[4] = {0x00, 0x00, 0x00, 0x01};

H264VideoSdpParser* H264VideoSdpParser::createNew(UsageEnvironment& env, FramedSource* inputSource, char const* sPropParameterSetsStr)
{
    return new H264VideoSdpParser(env, inputSource, sPropParameterSetsStr);
}

 H264VideoSdpParser::H264VideoSdpParser(UsageEnvironment& env, FramedSource* inputSource, char const* sPropParameterSetsStr)
  : FramedFilter(env, inputSource), injectedMetadataNALs(0), extradataSize(0)
{
    sPropRecords = parseSPropParameterSets(sPropParameterSetsStr, numSPropRecords);
}

H264VideoSdpParser::~H264VideoSdpParser() 
{
    
}

void H264VideoSdpParser::doGetNextFrame() 
{
    if (injectedMetadataNALs < numSPropRecords) {
        memmove(extradata + extradataSize, startCode, sizeof(startCode));
        memmove(extradata + sizeof(startCode) + extradataSize, sPropRecords[injectedMetadataNALs].sPropBytes, sPropRecords[injectedMetadataNALs].sPropLength);
        extradataSize += sizeof(startCode) + sPropRecords[injectedMetadataNALs].sPropLength;
        fNumTruncatedBytes = 0;
        injectedMetadataNALs++;
        afterGetting(this);
    } else {
        fInputSource->getNextFrame(fTo + sizeof(startCode), fMaxSize - sizeof(startCode), 
                                    afterGettingFrame, this, FramedSource::handleClosure, this);
    }  
}

void H264VideoSdpParser
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  H264VideoSdpParser* source = (H264VideoSdpParser*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264VideoSdpParser
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    memcpy(fTo, startCode, sizeof(startCode));
    fFrameSize = frameSize + sizeof(startCode);
    fNumTruncatedBytes = numTruncatedBytes;
    fPresentationTime = presentationTime;
    fDurationInMicroseconds = durationInMicroseconds;

    if (numTruncatedBytes > 0) {
        envir() << "H264VideoSdpParser error: Frame buffer too small\n";
    }

    afterGetting(this);
}