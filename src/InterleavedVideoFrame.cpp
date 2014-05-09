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

#include "InterleavedVideoFrame.hh"

InterleavedVideoFrame::InterleavedVideoFrame(unsigned int maxLength)
{
    width = 0;
    height = 0;
    bufferLen = 0;
    bufferMaxLen = maxLength;
    frameBuff = new unsigned char [bufferMaxLen]();
}

InterleavedVideoFrame::InterleavedVideoFrame(unsigned int width, unsigned height)
{
    this->width = width;
    this->height = height;
    bufferLen = 0;
    bufferMaxLen = width * height * BYTES_PER_PIXEL;
    frameBuff = new unsigned char [bufferMaxLen]();
}

unsigned char* InterleavedVideoFrame::getDataBuf()
{
    return frameBuff;
}

unsigned int InterleavedVideoFrame::getLength()
{
    return bufferLen;
}
    
unsigned int InterleavedVideoFrame::getMaxLength()
{
    return bufferMaxLen;
}
    
void InterleavedVideoFrame::setLength(unsigned int length)
{
    bufferLen = length;
}

bool InterleavedVideoFrame::isPlanar()
{
    return false;
}
      
