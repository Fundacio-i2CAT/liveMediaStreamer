/*
 *  Frame - AV Frame structure
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
 *  Authors: David Cassany <david.cassany@i2cat.net> 
 *           Marc Palau <marc.palau@i2cat.net>
 */

#include "Frame.hh"

Frame::Frame()
{
	originTime = std::chrono::system_clock::now();
}

void Frame::setPresentationTime(std::chrono::microseconds pTime)
{
    presentationTime = pTime;
}

void Frame::newOriginTime()
{
	originTime = std::chrono::system_clock::now();
}

void Frame::setOriginTime(std::chrono::system_clock::time_point orgTime)
{
    originTime = orgTime;
}

void Frame::setDuration(std::chrono::nanoseconds dur)
{
    duration = dur;
}
       
void Frame::setSequenceNumber(size_t seqNum)
{
    sequenceNumber = seqNum;
}