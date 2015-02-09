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
 *            David Cassany <david.cassany@i2cat.net>
 *            
 */

#ifndef _FILTER_MOCKUP_HH
#define _FILTER_MOCKUP_HH

#include <thread>
#include <chrono>
#include <random>

#include "Filter.hh"
#include "AVFramedQueue.hh"
#include "Frame.hh"

#define READERS 1
#define WRITERS 1


class FrameMock : public Frame {
public:
    ~FrameMock(){};
    virtual unsigned char *getDataBuf() {
        return buff;
    };
    
    static FrameMock* createNew() {
        return new FrameMock();
    }
    
    virtual unsigned char **getPlanarDataBuf() {return NULL;};
    virtual unsigned int getLength() {return 4;};
    virtual unsigned int getMaxLength() {return 4;};
    virtual void setLength(unsigned int length) {};
    virtual bool isPlanar() {return false;};
    
protected:
    unsigned char buff[4];
};

class AVFramedQueueMock : public AVFramedQueue 
{
public:
    AVFramedQueueMock(unsigned max_) : AVFramedQueue() {
        max = max_;
        config();
    };
    
protected:
    virtual bool config() {
        for (unsigned i=0; i<max; i++) {
                frames[i] = FrameMock::createNew();
        }
        return true;
    }
};

class BaseFilterMockup : public BaseFilter 
{
public:
    BaseFilterMockup(unsigned readers, unsigned writers) : BaseFilter(readers,writers) {};
    
protected:
    FrameQueue *allocQueue(int wId) {return new AVFramedQueueMock(4);};
    size_t processFrame() {return 20;};
    void doGetState(Jzon::Object &filterNode) {};
    void stop() {};

private:
    VCodecType codec;
};


class VideoFilterMockup : public BaseFilter 
{
public:
    VideoFilterMockup(VCodecType c) : BaseFilter(READERS,WRITERS) {codec = c;};
    
protected:
    FrameQueue *allocQueue(int wId) {return VideoFrameQueue::createNew(codec);};
    size_t processFrame() {return 20;};
    //Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false) {return NULL;};
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
    //Reader *setReader(int readerID, FrameQueue* queue, bool sharedQueue = false) {return NULL;};
    void doGetState(Jzon::Object &filterNode) {};
    void stop() {};

private:
    ACodecType codec;
};

class OneToOneFilterMockup : public OneToOneFilter 
{
public:
    OneToOneFilterMockup(size_t processTime_, size_t queueSize_, bool gotFrame_) : 
        OneToOneFilter(), processTime(processTime_), queueSize(queueSize_), gotFrame(gotFrame_) {};
    
    void setGotFrame(bool gotFrame_) {gotFrame = gotFrame_;};
    using BaseFilter::getReader;
    
protected:
    bool doProcessFrame(Frame *org, Frame *dst) {
        size_t realProcessTime;
        std::uniform_int_distribution<size_t> distribution(processTime/2, processTime*0.99);
        realProcessTime = distribution(generator);
        utils::debugMsg("Process time " + std::to_string(realProcessTime));
        std::this_thread::sleep_for(std::chrono::microseconds(realProcessTime));
        return gotFrame;
    }
    void doGetState(Jzon::Object &filterNode) {};
    void stop() {};

private:
    FrameQueue *allocQueue(int wId) {return new AVFramedQueueMock(queueSize);};
    
    std::default_random_engine generator;
    size_t processTime; //usec
    size_t queueSize;
    bool gotFrame;
};

#endif
