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

#include "VideoMixer.hh"
#include "../../AVFramedQueue.hh"
#include <iostream>
#include <chrono>

///////////////////////////////////////////////////
//                ChannelConfig Class            //
///////////////////////////////////////////////////

ChannelConfig::ChannelConfig() : width(1), height(1), x(0), y(0), layer(0), enabled(false), opacity(1.0) 
{

}

void ChannelConfig::config(float width, float height, float x, float y, int layer, bool enabled, float opacity)
{
    this->width = width;
    this->height = height;
    this->x = x;
    this->y = y;
    this->layer = layer;
    this->enabled = enabled;
    this->opacity = opacity;
}

///////////////////////////////////////////////////
//                VideoMixer Class               //
///////////////////////////////////////////////////

VideoMixer* VideoMixer::createNew(int inputChannels, int outWidth, int outHeight, std::chrono::microseconds fTime)
{
    if (outWidth <= 0 || outWidth > DEFAULT_WIDTH || outHeight <= 0 || outHeight > DEFAULT_HEIGHT) {
        utils::errorMsg("[VideoMixer] Error creating VideoMixer, output size range is  (0," + 
                         std::to_string(DEFAULT_WIDTH) + "]x(0," + std::to_string(DEFAULT_HEIGHT) + "]");
        return NULL;
    }

    if (fTime.count() < 0) {
        utils::errorMsg("[VideoMixer] Error creating VideoMixer, negative frame time is not valid");
        return NULL;
    }

    return new VideoMixer(inputChannels, outWidth, outHeight, fTime);
}

VideoMixer::VideoMixer(int inputChannels, 
                       int outWidth, int outHeight, std::chrono::microseconds fTime) :
ManyToOneFilter(inputChannels), outputWidth(outWidth), outputHeight(outHeight), maxChannels(inputChannels)
{
    setFrameTime(fTime);
    layoutImg = cv::Mat(outputHeight, outputWidth, CV_8UC3);
    initializeEventMap();
    fType = VIDEO_MIXER;
}

VideoMixer::~VideoMixer()
{
    for (auto it : channelsConfig) {
        delete it.second;
    }

    channelsConfig.clear();
}

FrameQueue* VideoMixer::allocQueue(struct ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, RAW, DEFAULT_RAW_VIDEO_FRAMES, RGB24);
}

bool VideoMixer::doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst)
{
    int frameNumber = orgFrames.size();
    std::chrono::microseconds outTs = std::chrono::microseconds(0);
    VideoFrame *vFrame;

    vFrame = dynamic_cast<VideoFrame*>(dst);

    if (!vFrame) {
        utils::errorMsg("[VideoMixer] Destination frame must be a VideoFrame");
        return false;
    }

    layoutImg.data = vFrame->getDataBuf();
    vFrame->setLength(layoutImg.step * outputHeight);
    vFrame->setSize(outputWidth, outputHeight);

    layoutImg = cv::Scalar(0, 0, 0);

    for (int lay=0; lay < maxChannels; lay++) {

        for (auto it : orgFrames) {

            if (channelsConfig[it.first]->getLayer() != lay) {
                continue;
            }

            if (!it.second || !channelsConfig[it.first]->isEnabled()) {
                frameNumber--;
                continue;
            }

            vFrame = dynamic_cast<VideoFrame*>(it.second);

            if (!vFrame) {
                utils::errorMsg("[VideoMixer] Origin frame must be a VideoFrame");
                return false;
            }

            pasteToLayout(it.first, vFrame);
            outTs = std::max(vFrame->getPresentationTime(), outTs);
            frameNumber--;
        }

        if (frameNumber <= 0) {
            break;
        }
    }

    dst->setConsumed(true);
    
    if (getFrameTime().count() <= 0) {
        dst->setPresentationTime(outTs);
        setSyncTs(outTs);
    } else {
        dst->setPresentationTime(getSyncTs());
    }

    return true;
}

bool VideoMixer::configChannel0(int id, float width, float height, float x, float y, int layer, bool enabled, float opacity)
{
    if (channelsConfig.count(id) <= 0) {
        return false;
    }

    if (x < 0 || y < 0 || width <= 0 || height <= 0 || opacity < 0 || opacity > 1.0) {
        utils::errorMsg("[VideoMixer] Error configuring channel. Incoherent values");
        return false;
    }

    if (x + width > 1 || y + height > 1) {
        utils::errorMsg("[VideoMixer] Error configuring channel. Position + size exceed layout edges");
        return false;
    }

    if (layer < 0 || layer > maxChannels) {
        utils::errorMsg("[VideoMixer] Error configuring channel. Layer value is not valid");
        return false;
    }

    channelsConfig[id]->config(width, height, x, y, layer, enabled, opacity);

    return true;
}

void VideoMixer::pasteToLayout(int frameID, VideoFrame* vFrame)
{
    ChannelConfig* chConfig = channelsConfig[frameID];
    cv::Mat img(vFrame->getHeight(), vFrame->getWidth(), CV_8UC3, vFrame->getDataBuf());

    cv::Size sz(chConfig->getWidth()*outputWidth, chConfig->getHeight()*outputHeight);

    int x = chConfig->getX()*outputWidth;
    int y = chConfig->getY()*outputHeight;

    int cropX = (vFrame->getWidth() - chConfig->getWidth()*outputWidth)/2;
    int cropY = (vFrame->getHeight() - chConfig->getHeight()*outputHeight)/2;

    if (cropX < 0 || cropX + sz.width > vFrame->getWidth() || cropY < 0 || cropY + sz.height > vFrame->getHeight()) {
        utils::errorMsg("[VideoMixer] Input size does not match with channel config");
        return;
    }

    if (chConfig->getOpacity() == 1) {
        img(cv::Rect(cropX, cropY, sz.width, sz.height)).copyTo(layoutImg(cv::Rect(x, y, sz.width, sz.height)));
    } else {
        addWeighted(
            img(cv::Rect(cropX, cropY, sz.width, sz.height)),
            chConfig->getOpacity(),
            layoutImg(cv::Rect(x, y, sz.width, sz.height)),
            1 - chConfig->getOpacity(),
            0.0,
            layoutImg(cv::Rect(x, y, sz.width, sz.height))
        );
    }
}

std::shared_ptr<Reader> VideoMixer::setReader(int readerId, FrameQueue* queue)
{
    if (readers.count(readerId) > 0) {
        return NULL;
    }

    std::shared_ptr<Reader> r(new Reader());
    readers[readerId] = r;

    channelsConfig[readerId] = new ChannelConfig();

    return r;
}

void VideoMixer::initializeEventMap()
{
    eventMap["configChannel"] = std::bind(&VideoMixer::configChannelEvent, this, std::placeholders::_1);
}

bool VideoMixer::configChannelEvent(Jzon::Node* params)
{
    if (!params) {
        utils::errorMsg("[VideoMixer::configChannelEvent] Params node missing");
        return false;
    }

    if (!params->Has("id") || !params->Has("width") || !params->Has("height") ||
            !params->Has("x") || !params->Has("y") || !params->Has("layer") ||
                !params->Has("enabled") || !params->Has("opacity")) {

        utils::errorMsg("[VideoMixer::configChannelEvent] Params node not complete");
        return false;
    }

    int id = params->Get("id").ToInt();
    float width = params->Get("width").ToFloat();
    float height = params->Get("height").ToFloat();
    float x = params->Get("x").ToFloat();
    float y = params->Get("y").ToFloat();
    int layer = params->Get("layer").ToInt();
    bool enabled = params->Get("enabled").ToBool();
    float opacity = params->Get("opacity").ToFloat();

    return configChannel0(id, width, height, x, y, layer, enabled, opacity);
}

void VideoMixer::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array jsonChannelConfigs;

    filterNode.Add("width", outputWidth);
    filterNode.Add("height", outputHeight);
    filterNode.Add("maxChannels", maxChannels);

    for (auto it : channelsConfig) {
        Jzon::Object chConfig;
        chConfig.Add("id", it.first);
        chConfig.Add("width", it.second->getWidth());
        chConfig.Add("height", it.second->getHeight());
        chConfig.Add("x", it.second->getX());
        chConfig.Add("y", it.second->getY());
        chConfig.Add("layer", it.second->getLayer());
        chConfig.Add("enabled", it.second->isEnabled());
        chConfig.Add("opacity", it.second->getOpacity());
        jsonChannelConfigs.Add(chConfig);
    }

    filterNode.Add("channels", jsonChannelConfigs);
}

bool VideoMixer::configChannel(int id, float width, float height, float x, float y, int layer, bool enabled, float opacity)
{
    Jzon::Object root, params;
    root.Add("action", "configChannel");
    params.Add("id", id);
    params.Add("width", width);
    params.Add("height", height);
    params.Add("x", x);
    params.Add("y", y);
    params.Add("layer", y);
    params.Add("enabled", enabled);
    params.Add("opacity", opacity);
    root.Add("params", params);

    Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}
