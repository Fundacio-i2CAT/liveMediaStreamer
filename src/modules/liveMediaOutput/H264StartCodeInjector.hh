/*
 *  H264StartCodeInjector.hh - H264 NAL Unit start code injector.
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

#ifndef _H264_START_CODE_INJECTOR_HH
#define _H264_START_CODE_INJECTOR_HH

#include "H264or5VideoStreamFramer.hh"
#define NAL_START_SIZE 4

class H264StartCodeInjector: public H264or5VideoStreamFramer {
public:
    static H264StartCodeInjector* createNew(UsageEnvironment& env, FramedSource* inputSource);

protected:
    H264StartCodeInjector(UsageEnvironment& env, FramedSource* inputSource);
    virtual ~H264StartCodeInjector();

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
};

#endif