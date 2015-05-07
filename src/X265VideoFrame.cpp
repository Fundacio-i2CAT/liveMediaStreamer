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

#include "X265VideoFrame.hh"

X265VideoFrame* X265VideoFrame::createNew()
{
    return new X265VideoFrame();
}

X265VideoFrame::X265VideoFrame() : X264or5VideoFrame(H265)
{   

}

X265VideoFrame::~X265VideoFrame()
{
    delete[] hdrNals;
    delete[] nals;
}
