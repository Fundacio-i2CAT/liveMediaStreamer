/*
 *  SlicedVideoFrameQueue - X265 Video circular buffer
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            
 */

#include "SlicedVideoFrameQueue.hh"
#include "Utils.hh"
#include <cstring>

SlicedVideoFrameQueue* SlicedVideoFrameQueue::createNew(struct ConnectionData cData,
        const StreamInfo *si, unsigned maxFrames, unsigned maxSliceSize)
{
    SlicedVideoFrameQueue* q;

    if (maxFrames <= 0 || maxSliceSize <= 0) {
        return NULL;
    }

    q = new SlicedVideoFrameQueue(cData, si, maxFrames);

    if (!q->setup(maxSliceSize)) {
        utils::errorMsg("SlicedVideoFrameQueue setup error");
        delete q;
        return NULL;
    }

    return q;
}

SlicedVideoFrameQueue::SlicedVideoFrameQueue(struct ConnectionData cData, const StreamInfo *si,
        unsigned maxFrames) : VideoFrameQueue(cData, si, maxFrames)
{
}

SlicedVideoFrameQueue::~SlicedVideoFrameQueue()
{
    delete inputFrame;
}

Frame* SlicedVideoFrameQueue::getRear()
{
    if ((rear + 1) % max == front){
        return NULL;
    }

    return inputFrame;
}

std::vector<int> SlicedVideoFrameQueue::addFrame()
{
    pushBackSliceGroup(inputFrame->getSlices(), inputFrame->getSliceNum());
    inputFrame->clear();
    
    std::vector<int> ret;
    
    for (auto& r : connectionData.readers){
        ret.push_back(r.rFilterId);
    }
    
    return ret;
}

Frame* SlicedVideoFrameQueue::forceGetRear()
{
    return inputFrame;
}

Frame* SlicedVideoFrameQueue::innerGetRear() 
{
    if ((rear + 1) % max == front){
        return NULL;
    }
    
    return frames[rear];
}

Frame* SlicedVideoFrameQueue::innerForceGetRear()
{
    Frame *frame;
    while ((frame = innerGetRear()) == NULL) {
        utils::debugMsg("Frame discarted by X264 Circular Buffer");
        flush();
    }
    return frame;
}

void SlicedVideoFrameQueue::innerAddFrame() 
{
    rear =  (rear + 1) % max;
}

bool SlicedVideoFrameQueue::setup(unsigned maxSliceSize)
{
    inputFrame = SlicedVideoFrame::createNew(streamInfo->video.codec);

    if (!inputFrame) {
        return false;
    }

    for (unsigned i=0; i < max; i++) {
        frames[i] = InterleavedVideoFrame::createNew(streamInfo->video.codec, maxSliceSize);

        if (!frames[i]) {
            return false;
        }
    }

    return true;
}

void SlicedVideoFrameQueue::pushBackSliceGroup(Slice* slices, int sliceNum) 
{
    Frame* frame;
    VideoFrame* vFrame;

    for (int i=0; i<sliceNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }

        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(inputFrame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), slices[i].getData(), slices[i].getDataSize());
        vFrame->setLength(slices[i].getDataSize());
        vFrame->setPresentationTime(inputFrame->getPresentationTime());
        vFrame->setOriginTime(inputFrame->getOriginTime());
        vFrame->setSize(inputFrame->getWidth(), inputFrame->getHeight());
        innerAddFrame();
    }
}      
