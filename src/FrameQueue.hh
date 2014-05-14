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
    virtual Frame *getRear();
    virtual Frame *getFront();
    virtual void addFrame();
    virtual void removeFrame();
    virtual void flush();
    int delay; //(ms)

protected:
    virtual bool config() {};
    Frame* frames[MAX_FRAMES];
    std::atomic<int> rear;
    std::atomic<int> front;
    std::atomic<int> elements;
    int max;
    system_clock::time_point currentTime;
    milliseconds enlapsedTime;

    
};

#endif
