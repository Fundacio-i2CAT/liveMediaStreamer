/*
 *  AVFramedQueue - A lock-free AV frame circular queue
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
 */

#ifndef _AV_FRAMED_QUEUE_HH
#define _AV_FRAMED_QUEUE_HH

#include "FrameQueue.hh"
#include "AudioFrame.hh"

/*! It is an abstract class that represents a discrete buffering structure. 
*   Each queue position is associated to a frame. It is implemented by VideoFrameQueue and AudioFrameQueue
*/
class AVFramedQueue : public FrameQueue {

public:
    /**
    * See FrameQueue::getRear
    */
    virtual Frame *getRear();

    /**
    * See FrameQueue::getFront
    */
    Frame *getFront();

    /**
    * See FrameQueue::adFrame
    */
    virtual int addFrame();

    /**
    * See FrameQueue::removeFrame
    */
    int removeFrame();

    /**
    * See FrameQueue::forceGetRear
    */
    virtual Frame *forceGetRear();

    /**
    * See FrameQueue::forceGetFront
    */
    Frame *forceGetFront();

    /**
    * See FrameQueue::getElements
    */
    const unsigned getElements();
    const uint8_t *getExtraData() const {return extradata;};
    int getExtraDataSize() const {return extradata_size;};

    virtual ~AVFramedQueue();

protected:
    AVFramedQueue(int wId, int rId, unsigned maxFrames);
    void flush();
    Frame* frames[MAX_FRAMES];
    unsigned max;
    const uint8_t *extradata;
    int extradata_size;
};

/*! It represents a video AVFramedQueue */

class VideoFrameQueue : public AVFramedQueue {

public:
    /**
    * Constructor wrapper that validates input parameters
    * @param wFId see FrameQueue::FrameQueue 
    * @param rFId see FrameQueue::FrameQueue 
    * @param codec video codec of the data stored in queue frames. See #VCodecType
    * @param maxFrames queue max frames
    * @param pixelFormat pixel format of the data stored in queue frames, mandatory if codec == RAW. See #PixType.
    * @return pointer to a new object or NULL if invalid parameters
    */
    static VideoFrameQueue* createNew(int wId, int rId, VCodecType codec, unsigned maxFrames, PixType pixelFormat = P_NONE, 
                                      const uint8_t *extradata = NULL, int extradata_size = 0);

    /**
    * @return codec 
    */
    const VCodecType getCodec() const {return codec;};

protected:
    VideoFrameQueue(int wId, int rId, VCodecType codec, unsigned maxFrames, PixType pixelFormat = P_NONE);
    VCodecType codec;
    PixType pixelFormat;

private:
    bool setup();

};

/*! It represents an audio AVFramedQueue */

class AudioFrameQueue : public AVFramedQueue {

public:
    /**
    * Constructor wrapper that validates input parameters
    * @param wFId see FrameQueue::FrameQueue 
    * @param rFId see FrameQueue::FrameQueue 
    * @param codec audio codec of the data stored in queue frames. See #ACodecType
    * @param maxFrames queue max frames
    * @param sampleRate sample rate the data stored in queue frames
    * @param channels channels of the data stored in queue frames
    * @param sFmt sample format of the data stored in queue frames. See #SampleFmt
    * @return pointer to a new object or NULL if invalid parameters
    */
    static AudioFrameQueue* createNew(int wId, int rId, ACodecType codec, unsigned maxFrames, 
                                      unsigned sampleRate = DEFAULT_SAMPLE_RATE, 
                                      unsigned channels = DEFAULT_CHANNELS, SampleFmt sFmt = S16,
                                      const uint8_t *extradata = NULL, int extradata_size = 0);
    
    /**
    * @return sample rate 
    */
    unsigned getSampleRate() const {return sampleRate;};
    
    /**
    * @return channels 
    */
    unsigned getChannels() const {return channels;};
    
    /**
    * @return codec 
    */
    const ACodecType getCodec() const {return codec;};
    
    /**
    * @return sample format 
    */
    const SampleFmt getSampleFmt() const {return sampleFormat;};

protected:
    AudioFrameQueue(int wId, int rId, ACodecType codec, unsigned maxFrames, SampleFmt sFmt, unsigned sampleRate, unsigned channels);

    ACodecType codec;
    SampleFmt sampleFormat;
    unsigned sampleRate;
    unsigned channels;

private:
    bool setup();

};

#endif
