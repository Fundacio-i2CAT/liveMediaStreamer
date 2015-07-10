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

/*! Class that contains one mixer channel configuration */

class ChannelConfig {

public:
    /**
    * Class constructor. It sets its attributes to default values
    * @see VideoMixer
    */
    ChannelConfig();

    /**
    * It sets class attributes. NOTE: it does not validate parameters coherence, so it must be done from outside
    * @param width Channel width in mixing width percentage (0.0,1.0]. Real width will be mixingWidth*width 
    * @param height Channel height in mixing height percentage (0.0,1.0]. Real height will be mixingHeight*height 
    * @param x Upper left corner X position in percentage (0.0,1.0]
    * @param y Upper left corner Y position in percentage (0.0,1.0]
    * @param layer Mixing layer. Range between 0 (rear) and MAX_CHANNELS(front)
    * @param enabled If true, channels is used for the mixing. If false, it is ignored. 
    * @param opacity Opacity value (0.0, 1.0)
    */
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

/*! Filter that mixes different video frames in one frame. Each channel is identified by and Id 
*   (which coincides with the reader associated to it) and has its own configuration 
*/

class VideoMixer : public ManyToOneFilter {

    public:
        /**
        * Class constructor wrapper used to validate input params
        * @param outWidth Mixed frames width in pixels
        * @param outHeight Mixed frames height in pixels
        * @param fTime Frame time in microseconds
        * @param fRole_ Filter role (NETWORK, MASTER, SLAVE)
        * @return Pointer to new object if succeed of NULL if not
        */
        static VideoMixer* createNew(FilterRole fRole_ = MASTER,
                   int inputChannels = VMIXER_MAX_CHANNELS,
                   int outputWidth = DEFAULT_WIDTH,
                   int outputHeight = DEFAULT_HEIGHT,
                   std::chrono::microseconds fTime = std::chrono::microseconds(0));
        /**
        * Class destructor
        */
        ~VideoMixer();

        /**
        * Configure channel, validating introduced data
        * @param id Channel id
        * @param width See ChannelConfig::config
        * @param height See ChannelConfig::config
        * @param x See ChannelConfig::config
        * @param y See ChannelConfig::config
        * @param layer See ChannelConfig::config
        * @param enabled See ChannelConfig::config
        * @param opacity See ChannelConfig::config
        */
        bool configChannel(int id, float width, float height, float x, float y, int layer, bool enabled, float opacity);

        /**
        * @return Mixing max channels
        */
        int getMaxChannels() {return maxChannels;};

    protected:
        //Protected for testing purposes
        VideoMixer(FilterRole fRole, int inputChannels,
                   int outWidth, int outHeight,
                   std::chrono::microseconds fTime);
        std::shared_ptr<Reader> setReader(int readerID, FrameQueue* queue);
        FrameQueue *allocQueue(int wFId, int rFId, int wId);
        bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);
        void doGetState(Jzon::Object &filterNode);
        bool configChannel0(int id, float width, float height, float x, float y, int layer, bool enabled, float opacity);

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
