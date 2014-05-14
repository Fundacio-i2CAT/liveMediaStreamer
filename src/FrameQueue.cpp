#include <stdlib.h>
#include "FrameQueue.hh"
#include "InterleavedAudioFrame.hh"
#include "InterleavedVideoFrame.hh"
#include <iostream>


Frame* FrameQueue::getRear() {
    if (elements >= max) {
        return NULL;
    }

    return frames[rear];
}

Frame* FrameQueue::getFront() {
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
void FrameQueue::addFrame() {
    rear =  (rear + 1) % max;
    ++elements;
}

//TODO: add exception if elements < 0
void FrameQueue::removeFrame() {
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

