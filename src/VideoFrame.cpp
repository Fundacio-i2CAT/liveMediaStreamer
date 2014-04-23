#include "VideoFrame.hh"


#define DEFAULT_HEIGHT 1080
#define DEFAULT_WIDTH 1920
#define BYTES_PER_PIXEL 3

VideoFrame::VideoFrame(){
    frameLength = BYTES_PER_PIXEL*DEFAULT_HEIGHT*DEFAULT_WIDTH;
    frameBuff = new unsigned char [frameLength]();
}

void VideoFrame::setLength(unsigned int length){
    frameLength = length;
}

void VideoFrame::setSize(unsigned int width, unsigned int height){
    this->height = height;
    this->width = width;
}

