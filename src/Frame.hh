#ifndef _FRAME_HH
#define _FRAME_HH

#include <sys/time.h>
#include <chrono>

#define DEFAULT_HEIGHT 1080
#define DEFAULT_WIDTH 1920
#define BYTES_PER_PIXEL 3

using namespace std::chrono;

class Frame {
    public:
        Frame(unsigned int maxLength);
              
        void setLength(unsigned int length);
        void setSize(unsigned int width, unsigned int height);
        void setPresentationTime(struct timeval pTime);
        void setUpdatedTime();
        
        unsigned int getWidth() {return width;};
        unsigned int getHeight() {return height;};
        unsigned int getLength() {return frameLength;};
        unsigned int getMaxLength() {return maxFrameLength;};
        struct timeval getPresentationTime() {return presentationTime;};
        system_clock::time_point getUpdatedTime() {return updatedTime;};
        unsigned char *getDataBuf(){return frameBuff;};
        
    private:
        unsigned int                frameLength;
        unsigned int                maxFrameLength;
        unsigned char               *frameBuff;
        unsigned int                width, height;
        struct timeval              presentationTime;
        system_clock::time_point    updatedTime;
};

#endif
