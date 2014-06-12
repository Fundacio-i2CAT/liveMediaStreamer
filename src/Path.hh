/*
 *  Path.hh - Path class
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

#ifndef _PATH_HH
#define _PATH_HH

#include "Filter.hh"

class Path {
public:
    Path(BaseFilter* origin, int orgWriterID); 
    bool connect(BaseFilter *destination, int dstReaderID);
    int getDstReaderID() {return dstReaderID;};
    void addWorker(Worker* worker);
    BaseFilter* getDestinationFilter() {return destination;};

protected:
    void addFilter(BaseFilter *filter);

private:
    BaseFilter *origin;
    BaseFilter *destination;
    std::vector<std::pair<BaseFilter*, Worker*> > filters;
    int orgWriterID;
    int dstReaderID;
};

class VideoDecoderPath : public Path {
public:
    VideoDecoderPath(BaseFilter *origin, int orgWriterID);
};

class AudioDecoderPath : public Path {
public:
    AudioDecoderPath(BaseFilter *origin, int orgWriterID);
};

class AudioEncoderPath : public Path {
public:
    AudioEncoderPath(BaseFilter *origin, int orgWriterID);
};


#endif
