/*
 *  VideoSplitter
 *	Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:  Alejandro Jiménez <ajimenezherrero@gmail.com>
 */

#include "VideoSplitter.hh"
#include "../../AVFramedQueue.hh"
#include <iostream>
#include <chrono> 

#include <opencv2/highgui/highgui.hpp>

///////////////////////////////////////////////////
//                 CropConfig Class              //
///////////////////////////////////////////////////

 CropConfig::CropConfig() : width(0), height(0), x(-1), y(-1)
{

}

void CropConfig::config(int width, int height, int x, int y)
{
    this->width = width;
    this->height = height;
    this->x = x;
    this->y = y;
    cv::Mat img(height, width, CV_8UC3);
    img.copyTo(this->crop);

}

///////////////////////////////////////////////////
//              VideoSplitter Class              //
///////////////////////////////////////////////////

VideoSplitter* VideoSplitter::createNew(int outputChannels, std::chrono::microseconds fTime)
{
	if (outputChannels <= 0){
		utils::errorMsg("[VideoSplitter] Error creating VideoSplitter, the minimum number of outputChannels is 1");
        return NULL;
	}
	if (fTime.count() < 0) {
        utils::errorMsg("[VideoSplitter] Error creating VideoSplitter, negative frame time is not valid");
        return NULL;
    }

    return new VideoSplitter(outputChannels, fTime);
}

VideoSplitter::~VideoSplitter()
{
	for(auto it :cropsConfig){
		delete it.second;
	}
	cropsConfig.clear();

	delete outputStreamInfo;
}

bool VideoSplitter::configCrop(int id, int width, int height, int x, int y)
{
	Jzon::Object root, params;
	root.Add("action", "configCrop");
	params.Add("id", id);
	params.Add("width", width);
	params.Add("height", height);
	params.Add("x", x);
	params.Add("y", y);
	root.Add("params", params);

	Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}

VideoSplitter::VideoSplitter(int outputChannels, std::chrono::microseconds fTime):
OneToManyFilter(), maxCrops(outputChannels)
{

	initializeEventMap();
	
	fType = SPLITTER;
	outputStreamInfo = new StreamInfo(VIDEO);
    outputStreamInfo->video.codec = RAW;
    outputStreamInfo->video.pixelFormat = RGB24;
    for(int it=1; it<=maxCrops; it++)
    {
    	specificWriterConfig(it);
    }
}

FrameQueue* VideoSplitter::allocQueue(ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_RAW_VIDEO_FRAMES);
}

bool VideoSplitter::doProcessFrame(Frame *org, std::map<int, Frame *> &dstFrames)
{
	bool processFrame = false;
	int xROI = -1;
	int yROI = -1;
	int widthROI = 0;
	int heightROI = 0;
	VideoFrame *vFrame;
	VideoFrame *vFrameDst;
	vFrame = dynamic_cast<VideoFrame*>(org);

	if(!vFrame){
		utils::errorMsg("[VideoSplitter] No origin frame");
		return false;
	}
	cv::Mat orgFrame(vFrame->getHeight(), vFrame->getWidth(), CV_8UC3, vFrame->getDataBuf());
	
	for (auto it : dstFrames){
		
		xROI = cropsConfig[it.first]->getX();
		yROI = cropsConfig[it.first]->getY();
		widthROI = cropsConfig[it.first]->getWidth();
		heightROI = cropsConfig[it.first]->getHeight();

		if(xROI+widthROI <= vFrame->getWidth() && yROI+heightROI <= vFrame->getHeight()){
			vFrameDst = dynamic_cast<VideoFrame*>(it.second);
			cropsConfig[it.first]->getCrop()->data = vFrameDst->getDataBuf();
			vFrameDst->setLength(widthROI * heightROI);
    		vFrameDst->setSize(widthROI, heightROI);
			orgFrame(cv::Rect(xROI, yROI, widthROI, heightROI)).copyTo(cropsConfig[it.first]->getCropRect(0, 0, widthROI, heightROI));
			it.second->setConsumed(true);
			it.second->setPresentationTime(vFrame->getPresentationTime());
			processFrame = true;
		} else {
			it.second->setConsumed(false);
		}
	}
	return processFrame;
}

void VideoSplitter::doGetState(Jzon::Object &filterNode)
{
	Jzon::Array jsonCropsConfigs;

	filterNode.Add("outputChannels", maxCrops);

	for(auto it : cropsConfig){
		Jzon::Object crConfig;
		crConfig.Add("id", it.first);
		crConfig.Add("width", it.second->getWidth());
        crConfig.Add("height", it.second->getHeight());
        crConfig.Add("x", it.second->getX());
        crConfig.Add("y", it.second->getY());
		jsonCropsConfigs.Add(crConfig);
	}

	filterNode.Add("crops", jsonCropsConfigs);
}

bool VideoSplitter::configCrop0(int id, int width, int height, int x, int y)
{
	if (cropsConfig.count(id) <= 0) {
        utils::errorMsg("[VideoSplitter] Error configuring crop. Incorrect Id " + std::to_string(id));
        return false;
    }

    if (x < 0 || y < 0 || width <= 0 || height <= 0) {
        utils::errorMsg("[VideoSplitter] Error configuring crop. Incoherent values");
        return false;
    }

    cropsConfig[id]->config(width, height, x, y);
    return true;
}

void VideoSplitter::initializeEventMap()
{
	eventMap["configCrop"] = std::bind(&VideoSplitter::configCropEvent, this, std::placeholders::_1);
}

bool VideoSplitter::configCropEvent(Jzon::Node* params)
{
	if (!params) {
        utils::errorMsg("[VideoSplitter::configCropEvent] Params node missing");
        return false;
    }

    if (!params->Has("id") || !params->Has("width") || !params->Has("height") ||
            !params->Has("x") || !params->Has("y")) {

        utils::errorMsg("[VideoSplitter::configCropEvent] Params node not complete");
        return false;
    }

    int id = params->Get("id").ToInt();
    float width = params->Get("width").ToInt();
    float height = params->Get("height").ToInt();
    float x = params->Get("x").ToInt();
    float y = params->Get("y").ToInt();

    return configCrop0(id, width, height, x, y);
}
        
bool VideoSplitter::specificWriterConfig(int writerID)
{
	cropsConfig[writerID] = new CropConfig();

	return true;
}

bool VideoSplitter::specificWriterDelete(int writerID)
{
	delete cropsConfig[writerID];
	cropsConfig.erase(writerID);

	return true;
}