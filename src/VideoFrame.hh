class VideoFrame {
    public:
        VideoFrame();
              
        void setFrame(unsigned char *buff, unsigned int length);
        void setFrameSize(unsigned int width, unsigned int height);
        
        unsigned int getWidth() {return width;};
        unsigned int getHeight() {return height;};
        unsigned int getLength() {return frameLength;};
        unsigned char *getFrame(){return frameBuff;};
        
    private:
        unsigned int        frameLength;
        unsigned char       *frameBuff;
        
        unsigned int        width, height;
};
