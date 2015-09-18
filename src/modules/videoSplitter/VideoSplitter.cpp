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

    return new VideoSplitter(outputChannels, fTime)
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

VideoSplitter::VideoSplitter(int outputChannels, std::chrono::microseconds fTime)
{

	initializeEventMap();

	outputStreamInfo = new StreamInfo(VIDEO);
    outputStreamInfo->video.codec = RAW;
    outputStreamInfo->video.pixelFormat = RGB24;
}

FrameQueue* VideoMixer::allocQueue(ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_RAW_VIDEO_FRAMES);
}

bool VideoSplitter::doProcessFrame(Frame *org, std::map<int, Frame *> dstFrames)
{
	bool processFrame = false;
	cv::Mat cropImage;
	VideoFrame *vFrame;
	vFrame = dynamic_cast<VideoFrame*>(org);

	if(!vFrame){
		utils::errorMsg();
		return false
	} else {
		cv:Mat orgFrame(vFrame->getHeight(), vFrame->getWidth(), CV_8UC3, vFrame->getDataBuf());
	}

	for (auto it : dstFrames){
		int xROI = cropsConfig[it.first]->getX();
		int yROI = cropsConfig[it.first]->getY();
		int widthROI = cropsConfig[it.first]->getWidth();
		int heightROI = cropsConfig[it.first]->getHeight();
		
		if(xROI+widthROI <= vFrame->getWidth() && yROI+heightROI <= vFrame->getHeight()){
			cv::Rect ROI(xROI, yROI, widthROI, heightROI);
			cropImage = orgFrame(ROI);
			cropImage.data = it.second->getDataBuf();
			it.second->setConsumed(true);
			it.second->setPresentationTime(vFrame->getPresentationTime());
			processFrame |= true;
		} else {
			it.second->setConsumed(false);
		}
	}
	return processFrame;
}

void doGetState(Jzon::Object &filterNode)
{
	Jzon::Array jsonCropsConfigs;

	filterNode.Ass("outputChannels", outputChannels);

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

bool configCrop0(int id, int width, int height, int x, int y)
{
	if (cropsConfig.count(id) <= 0) {
        utils::errorMsg("[VideoSplitter] Error configuring crop. Incorrect Id");
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

    return configChannel0(id, width, height, x, y);
}
        
bool VideoSplitter::specificWriterConfig(int writerID)
{
	cropsConfig[writerID] = new CropConfig();

	return true;
}

bool VideoSplitter::specificWriterDelete(int writerID)
{
	delete cropsConfig[writerID];
	cropsConfig.erease(writerID);

	return true;
}