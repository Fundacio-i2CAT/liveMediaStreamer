#include "VideoFrame.hh"

#define DEFAULT_HEIGTH 1080
#define DEFAULT_WIDTH 1920
#define BYTES_PER_PIXEL 3

VideoFrame::VideoFrame(){
    frameLength = BYTES_PER_PIXEL*DEFAULT_HEIGHT*DEFAULT_WIDTH;
    frameBuff = malloc(sizeof(unsigned char)*frameLength);
}

VideoFrame::setFrame(unsigned char *buff, unsigned int length){
    frameLength = length;
    memcpy(frameBuff, buff, length);
}

