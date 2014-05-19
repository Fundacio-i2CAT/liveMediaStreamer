#ifndef _FRAME_QUEUE_HH
#define _FRAME_QUEUE_HH

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#include <atomic>
#include <sys/time.h>
#include <chrono>
#include "Types.hh"

#define MAX_FRAMES 2000000

using namespace std::chrono;

class FrameQueue {

public:
    FrameQueue();
    virtual Frame *getRear() = 0;
    virtual Frame *getFront() = 0;
    virtual void addFrame() = 0;
    virtual void removeFrame() = 0;
    virtual void flush() = 0;
    virtual Frame *forceGetRear() = 0;
    virtual Frame *forceGetFront() = 0;
    virtual bool frameToRead() = 0;
    int delay; //(ms)

protected:
    virtual bool config() = 0;

    std::atomic<int> rear;
    std::atomic<int> front;
    system_clock::time_point currentTime;
    milliseconds enlapsedTime;

    
};

#endif
