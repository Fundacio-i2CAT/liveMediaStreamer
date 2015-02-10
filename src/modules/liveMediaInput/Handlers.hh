/*
 *  HandlersUtils.hh - Headers of several handlers
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *
 */

#ifndef _HANDLERS_HH
#define _HANDLERS_HH

#include <liveMedia.hh>
#include <string>

namespace handlers
{
    void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
    void subsessionAfterPlaying(void* clientData);
    void subsessionByeHandler(void* clientData);
    bool addSubsessionSink(UsageEnvironment& env, MediaSubsession *subsession);
};

#endif
