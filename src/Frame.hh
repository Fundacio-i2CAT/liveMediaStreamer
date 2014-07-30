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

#ifndef _FRAME_HH
#define _FRAME_HH

#include <sys/time.h>
#include <chrono>
#include "Types.hh"

using namespace std::chrono;

class Frame {
    public:
        Frame();
        virtual ~Frame() {};
              
        void setPresentationTime(struct timeval pTime);
        void setUpdatedTime();
        
        struct timeval getPresentationTime();
        system_clock::time_point getUpdatedTime();
        virtual unsigned char *getDataBuf() {return NULL;};
        virtual unsigned char **getPlanarDataBuf() {return NULL;};
        virtual unsigned int getLength() {return 0;}
        virtual unsigned int getMaxLength() {return 0;};
        virtual void setLength(unsigned int length) {};
        virtual bool isPlanar() {return false;};
        
    protected:
        struct timeval              presentationTime;
        system_clock::time_point    updatedTime;
};

#endif
