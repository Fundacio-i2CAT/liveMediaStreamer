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

SlicedVideoFrameQueue* SlicedVideoFrameQueue::createNew(VCodecType codec, unsigned maxFrames, unsigned maxSliceSize)
{
    SlicedVideoFrameQueue* q;

    if (maxFrames <= 0 || maxSliceSize <= 0) {
        return NULL;
    }

    q = new SlicedVideoFrameQueue(codec, maxFrames);

    if (!q->setup(maxSliceSize)) {
        utils::errorMsg("SlicedVideoFrameQueue setup error");
        delete q;
        return NULL;
    }

    return q;
}

SlicedVideoFrameQueue::SlicedVideoFrameQueue(VCodecType codec, unsigned maxFrames) : 
VideoFrameQueue(codec, maxFrames)
{

}

SlicedVideoFrameQueue::~SlicedVideoFrameQueue()
{
    delete inputFrame;
}

Frame* SlicedVideoFrameQueue::getRear()
{
    if (elements >= max) {
        return NULL;
    }

    return inputFrame;
}

void SlicedVideoFrameQueue::addFrame()
{
    pushBack();
}

Frame* SlicedVideoFrameQueue::forceGetRear()
{
    return inputFrame;
}

Frame* SlicedVideoFrameQueue::innerGetRear() 
{
    if (elements >= max) {
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
    ++elements;
}

bool SlicedVideoFrameQueue::setup(unsigned maxSliceSize)
{
    inputFrame = SlicedVideoFrame::createNew(codec, maxSliceSize);

    if (!inputFrame) {
        return false;
    }

    for (unsigned i=0; i < max; i++) {
        frames[i] = InterleavedVideoFrame::createNew(codec, maxSliceSize);

        if (!frames[i]) {
            return false;
        }
    }

    return true;
}

bool SlicedVideoFrameQueue::pushBack() 
{
    Frame* frame;
    VideoFrame* vFrame;
    int sliceNum;
    Slice* slices;

    sliceNum = inputFrame->getCopiedSliceNum();
    slices = inputFrame->getCopiedSlices();

    for (int i=0; i<sliceNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }

        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(inputFrame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), slices[i].getData(), slices[i].getDataSize());
        vFrame->setLength(slices[i].getDataSize());
        vFrame->setPresentationTime(inputFrame->getPresentationTime());
        vFrame->setSize(inputFrame->getWidth(), inputFrame->getHeight());
        vFrame->setDuration(inputFrame->getDuration());
        innerAddFrame();
    }

    sliceNum = inputFrame->getSliceNum();
    slices = inputFrame->getSlices();

    for (int i=0; i<sliceNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }

        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(inputFrame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), slices[i].getData(), slices[i].getDataSize());
        vFrame->setLength(slices[i].getDataSize());
        vFrame->setPresentationTime(inputFrame->getPresentationTime());
        vFrame->setSize(inputFrame->getWidth(), inputFrame->getHeight());
        vFrame->setDuration(inputFrame->getDuration());
        innerAddFrame();
    }
    
    inputFrame->clear();
    return true;
}   
