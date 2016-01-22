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

#define MAX_FRAMES 250 //!< The highest value for DEFAULT_AUDIO_FRAMES, DEFAULT_VIDEO_FRAMES, ...

#include "FrameQueue.hh"
#include "AudioFrame.hh"
#include "StreamInfo.hh"

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
    * See FrameQueue::addFrame
    */
    virtual std::vector<int> addFrame();

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
    unsigned getElements();

    /**
     * The maximum number of frames allocated during construction. Used for replication, basically.
     * @returns the #maxFrames parameter using at construction
     */
    unsigned getMaxFrames() const {return max;}

    virtual ~AVFramedQueue();

protected:
    AVFramedQueue(ConnectionData cData, const StreamInfo *si, unsigned maxFrames);
    void doFlush();
    Frame* frames[MAX_FRAMES];
    unsigned max;
};

/*! It represents a video AVFramedQueue */
class VideoFrameQueue : public AVFramedQueue {

public:
    /**
    * Constructor wrapper that validates input parameters
    * @param cData see FrameQueue::FrameQueue 
    * @param si see FrameQueue::FrameQueue
    * @param maxFrames queue max frames
    * @return pointer to a new object or NULL if invalid parameters
    */
    static VideoFrameQueue* createNew(ConnectionData cData, const StreamInfo *si,
            unsigned maxFrames);

protected:
    VideoFrameQueue(ConnectionData cData, const StreamInfo *si, unsigned maxFrames);

private:
    bool setup();

};

/*! It represents an audio AVFramedQueue */

class AudioFrameQueue : public AVFramedQueue {

public:
    /**
    * Constructor wrapper that validates input parameters
    * @param cData see FrameQueue::FrameQueue 
    * @param si see FrameQueue::FrameQueue
    * @param maxFrames queue max frames
    * @return pointer to a new object or NULL if invalid parameters
    */
    static AudioFrameQueue* createNew(ConnectionData cData, const StreamInfo *si,
            unsigned maxFrames);

protected:
    AudioFrameQueue(ConnectionData cData, const StreamInfo *si, unsigned maxFrames);

private:
    bool setup();

};

#endif
