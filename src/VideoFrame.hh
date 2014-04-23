class VideoFrame {
    public:
        VideoFrame();
              
        void setLength(unsigned int length);
        void setSize(unsigned int width, unsigned int height);
        
        unsigned int getWidth() {return width;};
        unsigned int getHeight() {return height;};
        unsigned int getLength() {return frameLength;};
        unsigned char *getDataBuf(){return frameBuff;};
        
    private:
        unsigned int        frameLength;
        unsigned char       *frameBuff;
        
        unsigned int        width, height;
};
