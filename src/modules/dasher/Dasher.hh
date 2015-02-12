/*
 *  Dasher.hh - Class that handles DASH sessions
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
 
#ifndef _DASHER_HH
#define _DASHER_HH

#include "QueueSource.hh"
#include "../../Filter.hh"
#include "../../IOInterface.hh"
#include "DashSegmenterVideoSource.hh"
#include "DashSegmenterAudioSource.hh"
#include "Connection.hh"

#include <map>
#include <string>

class Dasher : public TailFilter {

public:
    Dasher(int readersNum = MAX_READERS);
    ~Dasher();

    bool deleteReader(int id);
    void stop();
    bool doProcessFrame(std::map<int, Frame*> orgFrames);
      
private: 
    void initializeEventMap();
    Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false);

    std::map<int, DashSegmenter*> segmenters;
};

class DashSegmenter {

public:
    DashSegmenter();
    virtual bool manageFrame(Frame* frame) = 0;

protected:
    i2ctx* dashContext;

};

class DashVideoSegmenter : public DashSegmenter {

public:
    DashVideoSegmenter();
    bool manageFrame(Frame* frame);
    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t width, size_t height, size_t framerate);
};

class DashAudioSegmenter : public DashSegmenter {

public:
    DashAudioSegmenter();
    bool manageFrame(Frame* frame);
    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample);

};

#endif
