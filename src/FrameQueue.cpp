#include <stdlib.h>
#include "FrameQueue.hh"
#include "InterleavedAudioFrame.hh"
#include "InterleavedVideoFrame.hh"
#include <iostream>


FrameQueue* FrameQueue::createNew(VideoType type, unsigned delay) {
    return new FrameQueue(type, delay);
}

FrameQueue* FrameQueue::createNew(AudioType type, unsigned delay) {
    return new FrameQueue(type, delay);
}

FrameQueue::FrameQueue(VideoType type, unsigned delay) {
    rear = 0;
    front = 0;
    this->delay = delay; //(ms)
    aType = A_NONE;
    vType = type;


    configQueue(type);
}

FrameQueue::FrameQueue(AudioType type, unsigned delay) {
    rear = 0;
    front = 0;
    this->delay = delay; //(ms)
    aType = type;
    vType = V_NONE;

    configQueue(type);
}

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

bool FrameQueue::configQueue(VideoType type)
{
    switch(type) {
        case H264:
            max = FRAMES_H264;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedVideoFrame(LENGTH_H264);
            }
            break; 
        case RGB24:
            break; 
        case RGB32:
            break; 
        case YUV420:
            break;
        case YUV422:
            break;
        default:
        break;
    }
}

bool FrameQueue::configQueue(AudioType type)
{
    switch(type) {
        case OPUS:
            max = FRAMES_OPUS;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(2, 48000, MAX_SAMPLES_48K, OPUS_C, S16);
            }
            break; 
        case G711:
            max = FRAMES_PCMU;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(1, 8000, MAX_SAMPLES_8K, PCMU, U8);
            }
            break; 
        case PCMU_2CH_48K_16:
            max = FRAMES_PCMU;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(2, 48000, MAX_SAMPLES_48K, PCMU, S16);
            }
            break;        
        case PCMU_1CH_48K_16:
           max = FRAMES_PCMU;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(1, 48000, MAX_SAMPLES_48K, PCMU, S16);
            }
            break;         
        case PCMU_2CH_8K_16:
            max = FRAMES_PCMU;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(2, 8000, MAX_SAMPLES_8K, PCMU, S16);
            }
            break;          
        default:
        break;
    }

}

