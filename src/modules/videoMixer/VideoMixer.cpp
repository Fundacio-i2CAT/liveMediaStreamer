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

PositionSize::PositionSize(int width, int height, int x, int y, int layer)
{
    this->width = width;
    this->height = height;
    this->x = x;
    this->y = y;
    this->layer = layer;
    enabled = true;
}

VideoMixer::VideoMixer(int inputChannels) : ManyToOneFilter(inputChannels, true)
{
    outputWidth = DEFAULT_WIDTH;
    outputHeight = DEFAULT_HEIGHT;
    fType = VIDEO_MIXER;
    maxChannels = inputChannels;

    layoutImg = cv::Mat(outputHeight, outputWidth, CV_8UC3);

    initializeEventMap();
}

VideoMixer::VideoMixer(int inputChannels, int outputWidth, int outputHeight) :
ManyToOneFilter(inputChannels, true)
{
    this->outputWidth = outputWidth;
    this->outputHeight = outputHeight;
    fType = VIDEO_MIXER;
    maxChannels = inputChannels;

    layoutImg = cv::Mat(outputHeight, outputWidth, CV_8UC3);
    initializeEventMap();
}

FrameQueue* VideoMixer::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, 0, outputWidth, outputHeight, RGB24);
}

bool VideoMixer::doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst)
{
    int frameNumber = orgFrames.size();
    VideoFrame *vFrame;

    layoutImg.data = dst->getDataBuf();
    dst->setLength(layoutImg.step * outputHeight);
    dynamic_cast<VideoFrame*>(dst)->setSize(outputWidth, outputHeight);

    layoutImg = cv::Scalar(0, 0, 0);

    for (int lay=0; lay < MAX_LAYERS; lay++) {
        for (auto it : orgFrames) {
            if (positionAndSizes[it.first]->getLayer() != lay) {
                continue;
            }

            if (!it.second || !positionAndSizes[it.first]->isEnabled()) {
                frameNumber--;
                continue;
            }

            vFrame = dynamic_cast<VideoFrame*>(it.second);
            pasteToLayout(it.first, vFrame);
            frameNumber--;
        }

        if (frameNumber <= 0) {
            break;
        }
    }

    return true;
}

bool VideoMixer::setPositionSize(int id, float width, float height, float x, float y, int layer, bool enabled)
{
    //NOTE: w, h, x and y are set as layout size percentages

    auto it = positionAndSizes.find(id);

    if (it == positionAndSizes.end()) {
        return false;
    }

    if (x + width > 1 || y + height > 1) {
        return false;
    }

    it->second->setWidth(width*outputWidth);
    it->second->setHeight(height*outputHeight);
    it->second->setX(x*outputWidth);
    it->second->setY(y*outputHeight);
    it->second->setLayer(layer);
    it->second->setEnabled(enabled);

    return true;
}

void VideoMixer::pasteToLayout(int frameID, VideoFrame* vFrame)
{
    cv::Mat img(vFrame->getHeight(), vFrame->getWidth(), CV_8UC3, vFrame->getDataBuf());
    cv::Size sz(positionAndSizes[frameID]->getWidth(), positionAndSizes[frameID]->getHeight());
    int x = positionAndSizes[frameID]->getX();
    int y = positionAndSizes[frameID]->getY();

    if (img.size() != sz) {
        resize(img, layoutImg(cv::Rect(x, y, sz.width, sz.height)), sz, 0, 0, cv::INTER_LINEAR);
        return;
    }

    img.copyTo(layoutImg(cv::Rect(x, y, sz.width, sz.height)));
}

Reader* VideoMixer::setReader(int readerID, FrameQueue* queue)
{
    if (readers.count(readerID) < 0) {
        return NULL;
    }

    Reader* r = new Reader();
    readers[readerID] = r;

    PositionSize* posSize = new PositionSize(outputWidth, outputHeight, 0, 0, 0);

    positionAndSizes[readerID] = posSize;

    return r;
}

void VideoMixer::initializeEventMap()
{
     eventMap["setPositionSize"] = std::bind(&VideoMixer::setPositionSizeEvent, 
                                                this, std::placeholders::_1, std::placeholders::_2);

}

void VideoMixer::setPositionSizeEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    if (!params) {
        outputNode.Add("error", "Error changing master volume");
        return;
    }

    if (!params->Has("id") || !params->Has("width") || !params->Has("height") || 
                !params->Has("x") || !params->Has("y") || !params->Has("layer") || !params->Has("enabled")) {

        outputNode.Add("error", "Error setting position and size. Check parameters!");
        return;
    }

    int id = params->Get("id").ToInt();
    float width = params->Get("width").ToFloat();
    float height = params->Get("height").ToFloat();
    float x = params->Get("x").ToFloat();
    float y = params->Get("y").ToFloat();
    int layer = params->Get("layer").ToInt();
    bool enabled = params->Get("enabled").ToBool();

    if (!setPositionSize(id, width, height, x, y, layer, enabled)) {
        outputNode.Add("error", "Error setting position and size. Check parameters!");
    } else {
        outputNode.Add("error", Jzon::null);
    }
}

void VideoMixer::doGetState(Jzon::Object &filterNode)
{
    Jzon::Array jsonPosSize;

    filterNode.Add("width", outputWidth);
    filterNode.Add("height", outputHeight);
    filterNode.Add("maxChannels", maxChannels);

    for (auto it : positionAndSizes) {
        Jzon::Object posSize;
        posSize.Add("id", it.first);
        posSize.Add("width", it.second->getWidth());
        posSize.Add("height", it.second->getHeight());
        posSize.Add("x", it.second->getX());
        posSize.Add("y", it.second->getY());
        posSize.Add("layer", it.second->getLayer());
        jsonPosSize.Add(posSize);
    }

    filterNode.Add("channels", jsonPosSize);
}
