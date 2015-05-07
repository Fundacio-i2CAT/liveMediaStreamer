/*
 *  X265VideoFrame - X265 Video frame structure
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
 *  Authors: Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _X265_VIDEO_FRAME_HH
#define _X265_VIDEO_FRAME_HH

#include "VideoFrame.hh"

extern "C" {
    #include <x265.h>
}

/* This class represents an x265 video frame. It has an array of x265_nal structures, which are the output of x265 encoder and represent (each one) a NALU. */

class X265VideoFrame : public X264or5VideoFrame {

public:
    /*
    * Class constructor wrapper. It checks if maxLength is valid 
    * @return Pointer to new x265VideoFrame or NULL if failed allocating it 
    */
    static X265VideoFrame* createNew();

    /*
    * Class destructor 
    */
    ~X265VideoFrame();

    /*
    * @return A pointer to x265_nal array
    */
    x265_nal* getNals() {return nals;};
    
    /*
    * @return A pointer to x265_nal header array (contains non-VCL NALUs such as SPS, PPS, etc.)
    */
    x265_nal* getHdrNals() {return hdrNals;};

    void setNals(x265_nal* n){nals = n;};
    void setHdrNals(x265_nal* n){hdrNals = n;};
        
private:
    X265VideoFrame();
        
    x265_nal *nals;
    x265_nal *hdrNals;
};

#endif