#include <stdlib.h>
#include "FrameQueue.hh"
#include <iostream>


FrameQueue* FrameQueue::createNew(unsigned maxPos, unsigned maxBuffSize, unsigned delay) 
{
    return new FrameQueue(maxPos, maxBuffSize, delay);
}

FrameQueue::FrameQueue(unsigned maxPos, unsigned maxBuffSize, unsigned delay) 
{
    max = maxPos;
    rear = 0;
    front = 0;
    this->delay = delay; //(ms)
    for (int i = 0; i < max; i++) {
        frames[i] = new Frame(maxBuffSize);
    }
}

Frame* FrameQueue::getRear() 
{
    if (elements >= max) {
        return NULL;
    }

    return frames[rear];
}

Frame* FrameQueue::getFront() 
{
    if (elements <= 0) {
        return NULL;
    }
    
    currentTime = system_clock::now();
    enlapsedTime = duration_cast<milliseconds>(currentTime - frames[front]->getUpdatedTime());

    if(enlapsedTime.count() < delay) {
        return NULL;
    }

    return frames[front];
}

//TODO: add exception if elements > max
void FrameQueue::addFrame() 
{
    rear =  (rear + 1) % max;
    ++elements;
}

//TODO: add exception if elements < 0
void FrameQueue::removeFrame() 
{
    front = (front + 1) % max;
    --elements;
}

void FrameQueue::flush() 
{
    if (elements == max) {
        rear = (rear + (max - 1)) % max;
        --elements;
    }
}

Frame* FrameQueue::forceGetRear()
{
    Frame *frame;
    while ((frame = getRear()) == NULL) {
        std::cerr << "Frame discarted" << std::endl;
        flush();
    }
    return frame;
}

Frame* FrameQueue::forceGetFront()
{
    Frame *frame;
    if ((frame = getFront()) == NULL) {
        std::cerr << "Frame reused" << std::endl;
        frame = getOldie();
    }
    return frame;
}

//TODO shouldn't it be safer?
Frame* FrameQueue::getOldie()
{
    return frames[(front + (max - 1)) % max]; 
}