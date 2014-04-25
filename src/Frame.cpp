#include "Frame.hh"


Frame::Frame(unsigned int maxLength = BYTES_PER_PIXEL*DEFAULT_HEIGHT*DEFAULT_WIDTH)
{
    frameLength = 0;
    maxFrameLength = maxLength;
    frameBuff = new unsigned char [maxLength]();
}

void Frame::setLength(unsigned int length)
{
    frameLength = length;
}

void Frame::setSize(unsigned int width, unsigned int height)
{
    this->height = height;
    this->width = width;
}

void Frame::setPresentationTime(struct timeval pTime)
{
    presentationTime = pTime;
}

void Frame::setUpdatedTime()
{
    updatedTime = system_clock::now();
}

