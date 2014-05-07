#ifndef _FRAME_QUEUE_HH
#define _FRAME_QUEUE_HH

#ifndef _FRAME_HH
#include "Frame.hh"
#endif

#include <atomic>
#include <sys/time.h>
#include <chrono>

#define MAX_FRAMES 2000000

using namespace std::chrono;

class FrameQueue {

public:
    static FrameQueue* createNew(unsigned maxPos, unsigned maxBuffSize, unsigned delay);
    Frame *getRear();
    Frame *getFront();
    void addFrame();
    void removeFrame();
    void flush();
    int delay; //(ms)

protected:
    FrameQueue(unsigned maxPos, unsigned maxBuffSize, unsigned delay);
    ~FrameQueue();

private:
    Frame* frames[MAX_FRAMES];
    std::atomic<int> rear;
    std::atomic<int> front;
    std::atomic<int> elements;
    int max;
    system_clock::time_point currentTime;
    milliseconds enlapsedTime;
    
};

#endif
