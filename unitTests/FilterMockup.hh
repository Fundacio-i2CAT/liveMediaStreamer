/*
 *  FilterMockUp - A filter class mockup 
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *            
 */

#ifndef _FILTER_MOCKUP_HH
#define _FILTER_MOCKUP_HH

#include <thread>
#include <chronos>

#include "Filter.hh"
#include "AVFramedQueue.hh"

#define READERS 1
#define WRITERS 1

class VideoFilterMockup : public BaseFilter 
{
public:
    VideoFilterMockup(VCodecType c) : BaseFilter(READERS,WRITERS) {codec = c;};
    
protected:
    FrameQueue *allocQueue(int wId) {return VideoFrameQueue::createNew(codec);};
    size_t processFrame() {return 20;};
    Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false) {return NULL;};
    void doGetState(Jzon::Object &filterNode) {};
    void stop() {};

private:
    VCodecType codec;
};

class AudioFilterMockup : public BaseFilter 
{
public:
    AudioFilterMockup(ACodecType c) : BaseFilter(READERS,WRITERS) {codec = c;};
    
protected:
    FrameQueue *allocQueue(int wId) {return AudioFrameQueue::createNew(codec);};
    size_t processFrame() {return 8;};
    Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false) {return NULL;};
    void doGetState(Jzon::Object &filterNode) {};
    void stop() {};

private:
    ACodecType codec;
};

#endif
