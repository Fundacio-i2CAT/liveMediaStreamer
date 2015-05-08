/*
 *  X264VideoFrame - X264 Video frame structure
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors: Martin German <martin.german@i2cat.net>
 *           David Cassany <david.cassany@i2cat.net>
 *           Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _X264_VIDEO_FRAME_HH
#define _X264_VIDEO_FRAME_HH

#include "VideoFrame.hh"

extern "C" {
    #include <x264.h>
}

/* This class represents an x264 video frame. It has an array of x264_nal structures, which are the output of x264 encoder and represent (each one) a NALU. */

class X264VideoFrame : public X264or5VideoFrame {

public:
    /*
    * Class constructor wrapper. It checks if maxLength is valid
    * @return Pointer to new x264VideoFrame or NULL if failed allocating it
    */
    static X264VideoFrame* createNew();

    /*
    * Class destructor
    */
    ~X264VideoFrame();

    /*
    * @return A pointer to x264_nal array
    */
    x264_nal_t* getNals() {return nals;};

    /*
    * @return A pointer to x264_nal header array (contains non-VCL NALUs such as SPS, PPS, etc.)
    */
    x264_nal_t* getHdrNals() {return hdrNals;};
    void setNals(x264_nal_t* n){nals = n;};
    void setHdrNals(x264_nal_t* n){hdrNals = n;};

private:
    X264VideoFrame();

    x264_nal_t *nals;
    x264_nal_t *hdrNals;
};

#endif
