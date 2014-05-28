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

#include "../../Frame.hh"
#include "../../Filter.hh"

class PositionSize {

public:
    PositionSize(int width, int height, int x, int y, int layer);
    int getWidth() {return width;};
    int getHeight() {return height;};
    int getX() {return x;};
    int getY() {return y;};
    int getLayer() {return layer;};
    void setWidth(int width) {this->width = width;};
    void setHeight(int height) {this->height = height;};
    void setX(int x) {this->x = x;};
    void setY(int y) {this->y = y;};
    void setLayer(int layer) {this->layer = layer;};

private:
    int width;
    int height;
    int x;
    int y;
};

class VideoMixer : public ManyToOneFilter {
    
    public:
        VideoMixer(int inputChannels);
        VideoMixer(int inputChannels, int outputWidth, int outputHeight);
        FrameQueue *allocQueue();
        bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);

    private:
        std::map<int, positionSize> positionAndSizes;        
        int outputWidth;
        int outputHeight;
};


#endif