/*
 *  X265VideoCircularBuffer - X265 Video circular buffer
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

#include "X265VideoCircularBuffer.hh"
#include "Utils.hh"
#include <cstring>

X265VideoCircularBuffer* X265VideoCircularBuffer::createNew()
{
    X265VideoCircularBuffer* b = new X265VideoCircularBuffer();

    if (!b->setup()) {
        utils::errorMsg("X265VideoCircularBuffer setup error");
        delete b;
        return NULL;
    }

    return b;
}


X265VideoCircularBuffer::X265VideoCircularBuffer() : X264or5VideoCircularBuffer(H264)
{
    inputFrame = X265VideoFrame::createNew();
}

X265VideoCircularBuffer::~X265VideoCircularBuffer()
{
    delete inputFrame;
}

bool X265VideoCircularBuffer::pushBack() 
{
    Frame* frame;
    VideoFrame* vFrame;
    X265VideoFrame* x265inputFrame;
    int nalsNum;
    x265_nal* nals;

    x265inputFrame = dynamic_cast<X265VideoFrame*>(inputFrame);

    nalsNum = x265inputFrame->getHdrNalsNum();
    nals = x265inputFrame->getHdrNals();

    for (int i=0; i<nalsNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }
        
        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(x265inputFrame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), nals[i].payload, nals[i].sizeBytes);
        vFrame->setLength(nals[i].sizeBytes);
        vFrame->setPresentationTime(x265inputFrame->getPresentationTime());
        vFrame->setSize(x265inputFrame->getWidth(), x265inputFrame->getHeight());
        vFrame->setDuration(x265inputFrame->getDuration());
        innerAddFrame();
    }

    nalsNum = x265inputFrame->getNalsNum();
    nals = x265inputFrame->getNals();

    for (int i=0; i<nalsNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }

        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(x265inputFrame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), nals[i].payload, nals[i].sizeBytes);
        vFrame->setLength(nals[i].sizeBytes);
        vFrame->setPresentationTime(x265inputFrame->getPresentationTime());
        vFrame->setSize(x265inputFrame->getWidth(), x265inputFrame->getHeight());
        vFrame->setDuration(x265inputFrame->getDuration());
        innerAddFrame();
    }
    
    x265inputFrame->clearNalNum();
    
    return true;
}   