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

#ifndef _H264_VIDEO_SDP_PARSER_HH
#define _H264_VIDEO_SDP_PARSER_HH

#include <FramedFilter.hh>
#include <H264VideoRTPSource.hh> // for "parseSPropParameterSets()"

#define MAX_EXTRADATA_SIZE 128

/*! An AAC ADTS header parser, which constructs the Audio Specific Config string from extracted information */

class H264VideoSdpParser : public FramedFilter {

public:
    /**
    * Constructor wrapper
    * @param env Live555 environement
    * @param inputSource Input source, which contains AAC frames
    * @return Pointer to the object if succeded and NULL if not
    */
    static H264VideoSdpParser* createNew(UsageEnvironment& env, FramedSource* inputSource, char const* sPropParameterSetsStr);
    
    /**
    * @return Pointer to the parsed headers from sdp, mainly SPS and PPS
    */
    unsigned char* getExtradata() {return extradata;};
    
    /**
     * @return number of bytes filled in extradata buffer
     */
    unsigned getExtradataSize() {return extradataSize;};

protected:
    H264VideoSdpParser(UsageEnvironment& env, FramedSource* inputSource, char const* sPropParameterSetsStr);
    virtual ~H264VideoSdpParser();

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
    SPropRecord* sPropRecords;
    unsigned numSPropRecords;
    unsigned injectedMetadataNALs;
    
    unsigned char extradata[MAX_EXTRADATA_SIZE];
    unsigned extradataSize;
};

#endif