/*
 *  InterleavedVideoFrame - Interleaved video frame structure
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _INTERLEAVED_VIDEO_FRAME_HH
#define _INTERLEAVED_VIDEO_FRAME_HH

#include "VideoFrame.hh"

class InterleavedVideoFrame : public VideoFrame {
    public:
        InterleavedVideoFrame(unsigned int maxLength);
        InterleavedVideoFrame(unsigned int width, unsigned height);
        unsigned char* getDataBuf();
        unsigned int getLength();
        unsigned int getMaxLength();
        void setLength(unsigned int length);
        bool isPlanar();
              
    private:
        unsigned char *frameBuff;
        unsigned int bufferLen;
        unsigned int bufferMaxLen;
};

#endif