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
 *  Authors:    Marc Palau <marc.palau@i2cat.net>
 *              David Cassany <david.cassany@i2cat.net>
 *              Gerard Castillo <gerard.castillo@i2cat.net>
 *
 */

#ifndef _FILTER_MOCKUP_HH
#define _FILTER_MOCKUP_HH

#include <thread>
#include <chrono>
#include <random>
#include <cstring>

#include "Filter.hh"
#include "AVFramedQueueMockup.hh"
#include "AudioCircularBuffer.hh"
#include "VideoFrame.hh"
#include "FrameMockup.hh"
#include "StreamInfo.hh"

static const StreamInfo mockStreamInfo = {ST_NONE};

class BaseFilterMockup : public BaseFilter
{
public:
    BaseFilterMockup(unsigned readers, unsigned writers) : BaseFilter(readers, writers) {};

    using BaseFilter::getReader;

protected:
    FrameQueue *allocQueue(ConnectionData cData) {return new AVFramedQueueMock(cData, &mockStreamInfo, 4);};

    void doGetState(Jzon::Object &filterNode) {};

private:
    VCodecType codec;
    
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    bool runDoProcessFrame(std::map<int, Frame*> &oFrames, std::map<int, Frame*> &dFrames, std::vector<int> newFrames) {
        return true;
    };
};

class OneToOneFilterMockup : public OneToOneFilter
{
public:
    OneToOneFilterMockup(size_t queueSize_, bool gotFrame_,
                std::chrono::microseconds frameTime) :
                OneToOneFilter(), queueSize(queueSize_), gotFrame(gotFrame_) {
        setFrameTime(frameTime);
    };

    void setGotFrame(bool gotFrame_) {gotFrame = gotFrame_;};
    using BaseFilter::getReader;

protected:
    bool doProcessFrame(Frame *org, Frame *dst) {
        if (!org->isPlanar() && !dst->isPlanar() && org->getConsumed()){
            memcpy(dst->getDataBuf(), org->getDataBuf(), org->getLength());
            dst->setSequenceNumber(org->getSequenceNumber());
            dst->setLength(org->getLength());
            dst->setConsumed(gotFrame);
        } 
        
        return gotFrame;
    }
    void doGetState(Jzon::Object &filterNode) {};

private:
    virtual FrameQueue *allocQueue(ConnectionData cData) {return new AVFramedQueueMock(cData, &mockStreamInfo, queueSize);};
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    std::default_random_engine generator;
    size_t queueSize;
    bool gotFrame;
};

class OneToManyFilterMockup : public OneToManyFilter
{
public:
    OneToManyFilterMockup(unsigned maxWriters, size_t queueSize_, bool gotFrame_,
                         std::chrono::microseconds frameTime) :
        OneToManyFilter(maxWriters),
        queueSize(queueSize_), gotFrame(gotFrame_) {
            setFrameTime(frameTime);
        };

    void setGotFrame(bool gotFrame_) {gotFrame = gotFrame_;};
    using BaseFilter::getReader;

protected:
    bool doProcessFrame(Frame *org, std::map<int, Frame *> &dstFrames) {
        for (auto dst : dstFrames) {
            dst.second->setConsumed(gotFrame);
        }
        return gotFrame;
    }
    void doGetState(Jzon::Object &filterNode) {};

private:
    virtual FrameQueue *allocQueue(ConnectionData cData) {
        return new AVFramedQueueMock(cData, &mockStreamInfo, queueSize);
    };
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    std::default_random_engine generator;
    size_t queueSize;
    bool gotFrame;
};

class VideoFilterMockup : public OneToOneFilterMockup
{
public:
    VideoFilterMockup(VCodecType c) : OneToOneFilterMockup(4, true, std::chrono::microseconds(40000))  {
        outputStreamInfo = new StreamInfo(VIDEO);
        outputStreamInfo->video.codec = c;
        outputStreamInfo->setCodecDefaults();
    };

    ~VideoFilterMockup() { delete outputStreamInfo; }

protected:
    FrameQueue *allocQueue(ConnectionData cData) {
        return VideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_VIDEO_FRAMES);
    };

private:
    StreamInfo *outputStreamInfo;
};

class AudioFilterMockup : public OneToOneFilterMockup
{
public:
    AudioFilterMockup(ACodecType c) : OneToOneFilterMockup(4, true, std::chrono::microseconds(0)) {
        outputStreamInfo = new StreamInfo(AUDIO);
        outputStreamInfo->audio.codec = c;
        outputStreamInfo->setCodecDefaults();
    };

    ~AudioFilterMockup() { delete outputStreamInfo; }

protected:
    FrameQueue *allocQueue(struct ConnectionData cData) {
        return AudioFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_AUDIO_FRAMES);
    };

private:
    StreamInfo *outputStreamInfo;
};


class HeadFilterMockup : public HeadFilter
{
public:
    HeadFilterMockup() : HeadFilter(), srcFrame(NULL), newFrame(false) {};
    
    bool inject(Frame *f){
        if (!f || newFrame){
            return false;
        }
        
        srcFrame = f;
        newFrame = true;
        
        return true;
    }
    
    void doGetState(Jzon::Object &filterNode){};

protected:
    bool doProcessFrame(std::map<int, Frame*> &dstFrames) {
        if (!newFrame){
            return false;
        }
        
        bool gotFrame = false;
        newFrame = false;
               
        for (auto it : dstFrames){
            if (!it.second->isPlanar()){
                memcpy(it.second->getDataBuf(), srcFrame->getDataBuf(), srcFrame->getLength());
                it.second->setSequenceNumber(srcFrame->getSequenceNumber());
                it.second->setLength(srcFrame->getLength());
                it.second->setConsumed(true);
                gotFrame = true;
            }
        }
        return gotFrame;
    }
        
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    Frame* srcFrame;
    bool newFrame;
    
private:
    FrameQueue *allocQueue(struct ConnectionData cData) {
        return new AVFramedQueueMock(cData, &mockStreamInfo, 4);
    };
};

class TailFilterMockup : public TailFilter
{
public:
    TailFilterMockup() : TailFilter(), frames(0), frame(NULL), newFrame(false) {
        frame = FrameMock::createNew(0);
    };
    
    Frame* extract(){
        if (!newFrame){
            return NULL;
        }
        newFrame = false;
        return frame;
    }
    
    size_t getFrames() {return frames;};
    
    void doGetState(Jzon::Object &filterNode){};

protected:
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> /*newFrames*/) { 
        bool gotframe = false;
        for (auto it : orgFrames){
            if (!it.second->isPlanar() && it.second->getConsumed()){
                memcpy(frame->getDataBuf(), it.second->getDataBuf(), it.second->getLength());
                frame->setSequenceNumber(it.second->getSequenceNumber());
                frame->setLength(it.second->getLength());
                frames++;
                newFrame = true;
                gotframe = true;
            }
        }
        
        return gotframe;
    }
            
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    size_t frames;
    Frame* frame;
    bool newFrame;
    
};

class VideoHeadFilterMockup : public HeadFilter
{
public:
    VideoHeadFilterMockup(VCodecType c, PixType pix = P_NONE) : HeadFilter(), srcFrame(NULL) {
        outputStreamInfo = new StreamInfo (VIDEO);
        outputStreamInfo->video.codec = c;
        outputStreamInfo->video.pixelFormat = pix;
    };

    ~VideoHeadFilterMockup() { delete outputStreamInfo; }

    bool inject(InterleavedVideoFrame* frame){
        if (! frame || frame->getCodec() != outputStreamInfo->video.codec || 
            frame->getPixelFormat() != outputStreamInfo->video.pixelFormat){
            return false;
        }
        
        srcFrame = frame;
        
        return true;
    }
    
    void doGetState(Jzon::Object &filterNode){};
    
protected:
    bool doProcessFrame(std::map<int, Frame*> &dstFrames) {
        // There is only one frame in the map
        Frame *dst = dstFrames.begin()->second;
        InterleavedVideoFrame *dstFrame;
        
        if ((dstFrame = dynamic_cast<InterleavedVideoFrame*>(dst)) != NULL){
            memmove(dstFrame->getDataBuf(), srcFrame->getDataBuf(), sizeof(unsigned char)*srcFrame->getLength());
            
            dstFrame->setLength(srcFrame->getLength());
            dstFrame->setSize(srcFrame->getWidth(), srcFrame->getHeight());
            dstFrame->setPresentationTime(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
            dstFrame->setOriginTime(srcFrame->getOriginTime());
            dstFrame->setPixelFormat(srcFrame->getPixelFormat());
            dstFrame->setConsumed(true);
            return true;
        }
        
        return false;
    }

private:
    FrameQueue *allocQueue(struct ConnectionData cData) {
        return VideoFrameQueue::createNew(cData, outputStreamInfo, 10);
    };
        
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    InterleavedVideoFrame* srcFrame;
    StreamInfo *outputStreamInfo;
};

class AudioHeadFilterMockup : public HeadFilter
{
public:
    AudioHeadFilterMockup(unsigned ch, unsigned sRate, SampleFmt fmt) : HeadFilter(), srcFrame(NULL) {
        outputStreamInfo = new StreamInfo (AUDIO);
        outputStreamInfo->audio.channels = ch;
        outputStreamInfo->audio.sampleRate = sRate;
        outputStreamInfo->audio.sampleFormat = fmt;
    };

    ~AudioHeadFilterMockup() { delete outputStreamInfo; }

    bool inject(PlanarAudioFrame* frame) 
    {
        if (!frame || frame->getChannels() != outputStreamInfo->audio.channels ||
                frame->getSampleRate() != outputStreamInfo->audio.sampleRate ||
                frame->getSampleFmt() != outputStreamInfo->audio.sampleFormat) {
            return false;
        }

        srcFrame = frame;
        return true;
    }
    
    void doGetState(Jzon::Object &filterNode){};
    
protected:
    bool doProcessFrame(std::map<int, Frame*> &dstFrames) {
        // There is only one frame in the map
        Frame *dst = dstFrames.begin()->second;
        PlanarAudioFrame *dstFrame;
        
        dstFrame = dynamic_cast<PlanarAudioFrame*>(dst);

        if (!dstFrame) {
            return false;
        }

        for (unsigned i = 0; i < outputStreamInfo->audio.channels; i++) {
            memmove(dstFrame->getPlanarDataBuf()[i], srcFrame->getPlanarDataBuf()[i], srcFrame->getLength());
        }
        
        dstFrame->setLength(srcFrame->getLength());
        dstFrame->setSamples(srcFrame->getSamples());
        dstFrame->setChannels(srcFrame->getChannels());
        dstFrame->setSampleRate(srcFrame->getSampleRate());
        dstFrame->setPresentationTime(srcFrame->getPresentationTime());
        dstFrame->setOriginTime(srcFrame->getOriginTime());
        dstFrame->setConsumed(true);
        return true;
    }

private:
    FrameQueue *allocQueue(struct ConnectionData cData) {
        return AudioCircularBuffer::createNew(cData, outputStreamInfo->audio.channels,
                outputStreamInfo->audio.sampleRate, DEFAULT_BUFFER_SIZE,
                outputStreamInfo->audio.sampleFormat, std::chrono::milliseconds(0));
    };
        
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};
    
    PlanarAudioFrame* srcFrame;
    StreamInfo *outputStreamInfo;
};

class VideoTailFilterMockup : public TailFilter
{
public:
    VideoTailFilterMockup(): TailFilter(), oFrame(NULL), newFrame(false){};

    InterleavedVideoFrame* extract(){   
        if (newFrame){
            newFrame = false;
            return oFrame;
        } else {
            return NULL;
        }
    }
    
    ~VideoTailFilterMockup() {
        if (oFrame) {
            delete oFrame;
        }
    }
    void doGetState(Jzon::Object &filterNode){};
    
protected:
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> /*newFrame*/) {
        InterleavedVideoFrame *orgFrame;
 
        if ((orgFrame = dynamic_cast<InterleavedVideoFrame*>(orgFrames.begin()->second)) != NULL){
            if (!oFrame){
                oFrame = InterleavedVideoFrame::createNew(orgFrame->getCodec(), 
                                                          DEFAULT_WIDTH, DEFAULT_HEIGHT, orgFrame->getPixelFormat());
            }
            
            memmove(oFrame->getDataBuf(), orgFrame->getDataBuf(), sizeof(unsigned char)*orgFrame->getLength());
            
            oFrame->setLength(orgFrame->getLength());
            oFrame->setSize(orgFrame->getWidth(), orgFrame->getHeight());
            oFrame->setPresentationTime(orgFrame->getPresentationTime());
            oFrame->setOriginTime(orgFrame->getOriginTime());
            oFrame->setPixelFormat(orgFrame->getPixelFormat());
            oFrame->setSequenceNumber(orgFrame->getSequenceNumber());
            
            newFrame = true;
            
            return true;
        }
        
        return false;
    }
    

private:
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    InterleavedVideoFrame* oFrame;
    bool newFrame;
};

class AudioTailFilterMockup : public TailFilter
{
public:
    AudioTailFilterMockup(): TailFilter(), oFrame(NULL), newFrame(false){};

    PlanarAudioFrame* extract(){
        if (newFrame){
            newFrame = false;
            return oFrame;
        } else {
            return NULL;
        }
    }

    ~AudioTailFilterMockup() {
        if (oFrame) {
            delete oFrame;
        }
    }

    void doGetState(Jzon::Object &filterNode){};

protected:
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> /*newFrame*/) {
        PlanarAudioFrame *orgFrame;

        if ((orgFrame = dynamic_cast<PlanarAudioFrame*>(orgFrames.begin()->second)) != NULL){
            if (!oFrame){
                oFrame = PlanarAudioFrame::createNew(
                        orgFrame->getChannels(),
                        orgFrame->getSampleRate(),
                        orgFrame->getMaxSamples(),
                        orgFrame->getCodec(),
                        orgFrame->getSampleFmt());
            }

            unsigned char **orgData = orgFrame->getPlanarDataBuf();
            unsigned char **oData = oFrame->getPlanarDataBuf();

            for (unsigned i=0; i<orgFrame->getChannels(); i++) {
                memmove(oData[i], orgData[i], sizeof(unsigned char)*orgFrame->getLength());
            }

            oFrame->setPresentationTime(orgFrame->getPresentationTime());
            oFrame->setOriginTime(orgFrame->getOriginTime());
            oFrame->setSequenceNumber(orgFrame->getSequenceNumber());
            oFrame->setChannels(orgFrame->getChannels());
            oFrame->setSampleRate(orgFrame->getSampleRate());
            oFrame->setSamples(orgFrame->getSamples());
            oFrame->setLength(orgFrame->getLength());

            newFrame = true;

            return true;
        }

        return false;
    }


private:
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};

    PlanarAudioFrame* oFrame;
    bool newFrame;
};

#endif
