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
    Frame *forceGetRear();
    Frame *getFront();
    Frame *forceGetFront();
    void addFrame();
    void removeFrame();
    void flush();
    int delay; //(ms)
    const int getElements() {return elements;};
    bool frameToRead();

protected:
    FrameQueue(unsigned maxPos, unsigned maxBuffSize, unsigned delay);
    ~FrameQueue();

private:
    Frame *getOldie();
    
    Frame* frames[MAX_FRAMES];
    std::atomic<int> rear;
    std::atomic<int> front;
    std::atomic<int> elements;
    int max;
    system_clock::time_point currentTime;
    milliseconds enlapsedTime;
};

#endif
