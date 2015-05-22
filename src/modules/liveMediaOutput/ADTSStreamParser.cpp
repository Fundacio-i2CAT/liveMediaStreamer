/*
 *  ADTSStreamParser.cpp - AAC ADTS header parser.
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

#include "ADTSStreamParser.hh"
#include "../../Utils.hh"
#include <iostream>

#define ADTS_FIRST_RESERVED_BYTE 0XFF
#define ADTS_SECOND_RESERVED_BYTE 0XF1
#define ADTS_HEADER_LENGTH 7

ADTSStreamParser* ADTSStreamParser::createNew(UsageEnvironment& env, FramedSource* inputSource)
{
    return new ADTSStreamParser(env, inputSource);
}

ADTSStreamParser::ADTSStreamParser(UsageEnvironment& env, FramedSource* inputSource) : 
FramedFilter(env, inputSource)
{
    configString = new unsigned char[2]();
}

ADTSStreamParser::~ADTSStreamParser() {
}

void ADTSStreamParser::doGetNextFrame() 
{
    fInputSource->getNextFrame(fTo, fMaxSize, afterGettingFrame, this, FramedSource::handleClosure, this);
}

void ADTSStreamParser
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  ADTSStreamParser* source = (ADTSStreamParser*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void ADTSStreamParser
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    if (!updateConfigString(fTo, frameSize)) {
        utils::errorMsg("Error parsing ADTS header: cannot update AAC SDP config string");
    }

    fFrameSize = frameSize;
    fNumTruncatedBytes = numTruncatedBytes;
    fPresentationTime = presentationTime;
    fDurationInMicroseconds = durationInMicroseconds;
    afterGetting(this);
}

bool ADTSStreamParser::updateConfigString(unsigned char* data, unsigned size)
{
    unsigned char profile;
    unsigned char audioObjectType;
    unsigned char samplingFrequencyIndex;
    unsigned char channelConfiguration;

    if (!data || size < ADTS_HEADER_LENGTH) {
        utils::errorMsg("ADTS header not valid - check data buffer and its header length");
        return false;
    }

    if (data[0] != ADTS_FIRST_RESERVED_BYTE || data[1] != ADTS_SECOND_RESERVED_BYTE) {
        utils::errorMsg("ADTS header not valid - check reserved bytes");
        return false;
    }

    profile = getProfileFromADTSHeader(data);
    audioObjectType = profile + 1;
    samplingFrequencyIndex = getSamplingFreqIdxFromADTSHeader(data);
    channelConfiguration = getChannelConfFromADTSHeader(data);

    configString[0] = getMetadata1stByte(audioObjectType, samplingFrequencyIndex);
    configString[1] = getMetadata2ndByte(samplingFrequencyIndex, channelConfiguration);

    return true;
}

unsigned char ADTSStreamParser::getProfileFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] >> 6) & 0x03;
}

unsigned char ADTSStreamParser::getSamplingFreqIdxFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] >> 2) & 0x0F;
}

unsigned char ADTSStreamParser::getChannelConfFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] & 0x01) | ((adtsHeader[3] >> 6) & 0x03);
}

unsigned char ADTSStreamParser::getMetadata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex)
{
    return (audioObjectType<<3) | (samplingFrequencyIndex>>1);
}

unsigned char ADTSStreamParser::getMetadata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration)
{
    return (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
}
