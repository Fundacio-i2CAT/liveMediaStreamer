/*
 *  VideoMixer - Video mixer structure
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _VIDEO_MIXER_HH
#define _VIDEO_MIXER_HH

#include "../../VideoFrame.hh"
#include "../../Filter.hh"
#include <opencv/cv.hpp>


#define VMIXER_MAX_CHANNELS 16

class ChannelConfig {

public:
    ChannelConfig(float width, float height, float x, float y, int layer);
    void config(float width, float height, float x, float y, int layer, bool enabled, float opacity);

    float getWidth() {return width;};
    float getHeight() {return height;};
    float getX() {return x;};
    float getY() {return y;};
    int getLayer() {return layer;};
    float getOpacity() {return opacity;};
    bool isEnabled() {return enabled;};

private:
    float width;
    float height;
    float x;
    float y;
    int layer;
    bool enabled;
    float opacity;
};

class VideoMixer : public ManyToOneFilter {

    public:
        VideoMixer(int outWidth = DEFAULT_WIDTH,
                   int outHeight = DEFAULT_HEIGHT,
                   std::chrono::microseconds fTime = std::chrono::microseconds(0), 
                   FilterRole fRole_ = MASTER);
        ~VideoMixer();
        FrameQueue *allocQueue(int wId);
        bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);
        Reader* setReader(int readerID, FrameQueue* queue);
        bool configChannel(int id, float width, float height, float x, float y, int layer, bool enabled, float opacity);

    protected:
        void doGetState(Jzon::Object &filterNode);

    private:
        void initializeEventMap();
        void pasteToLayout(int frameID, VideoFrame* vFrame);

        bool configChannelEvent(Jzon::Node* params);

        std::map<int, ChannelConfig*> channelsConfig;
        int outputWidth;
        int outputHeight;
        cv::Mat layoutImg;
        int maxChannels;
};


#endif
