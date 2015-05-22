/*
 *  ADTSStreamParser.hh - AAC ADTS header parser.
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

#ifndef _ADTS_STREAM_PARSER_HH
#define _ADTS_STREAM_PARSER_HH

#include "FramedFilter.hh"

class ADTSStreamParser: public FramedFilter {
public:
    static ADTSStreamParser* createNew(UsageEnvironment& env, FramedSource* inputSource);
    unsigned char* getConfigString(){return configString;};

protected:
    ADTSStreamParser(UsageEnvironment& env, FramedSource* inputSource);
    virtual ~ADTSStreamParser();

protected:
    void doGetNextFrame();

protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame1(unsigned frameSize,
                            unsigned numTruncatedBytes,
                            struct timeval presentationTime,
                            unsigned durationInMicroseconds);
private:
    bool updateConfigString(unsigned char* data, unsigned size);
    unsigned char getProfileFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getSamplingFreqIdxFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getChannelConfFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getMetadata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex);
    unsigned char getMetadata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration);
    
    unsigned char* configString;

};

#endif