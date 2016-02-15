/*
 *  VideoSplitter
 *	Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:  Alejandro Jiménez <alejandro.jimenez@i2cat.net>
 */

#include "VideoSplitter.hh"
#include "../../AVFramedQueue.hh"
#include <iostream>
#include <chrono> 

///////////////////////////////////////////////////
//                 CropConfig Class              //
///////////////////////////////////////////////////

CropConfig::CropConfig() : width(0), height(0), x(-1), y(-1), degree(0)
{

}

void CropConfig::config(int width, int height, int x, int y, int degree)
{
    this->width = width;
    this->height = height;
    this->x = x;
    this->y = y;
    this->degree = degree;
    cv::Mat img(height, width, CV_8UC3);
    img.copyTo(this->crop);

}

///////////////////////////////////////////////////
//              VideoSplitter Class              //
///////////////////////////////////////////////////

VideoSplitter* VideoSplitter::createNew(std::chrono::microseconds fTime)
{
	if (fTime.count() < 0) {
        utils::errorMsg("[VideoSplitter] Error creating VideoSplitter, negative frame time is not valid");
        return NULL;
    }

    return new VideoSplitter(fTime);
}

VideoSplitter::~VideoSplitter()
{
	for(auto it :cropsConfig){
		delete it.second;
	}
	cropsConfig.clear();

	delete outputStreamInfo;
}

bool VideoSplitter::configCrop(int id, int width, int height, int x, int y, int degree)
{
	Jzon::Object root, params;
	root.Add("action", "configCrop");
	params.Add("id", id);
	params.Add("width", width);
	params.Add("height", height);
	params.Add("x", x);
	params.Add("y", y);
	params.Add("degree", degree);
	root.Add("params", params);

	Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}

bool VideoSplitter::configure(int fTime)
{
	Jzon::Object root, params;
	root.Add("action", "configure");
	params.Add("fTime", fTime);
	root.Add("params", params);
	Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e); 
    return true;
}


VideoSplitter::VideoSplitter(std::chrono::microseconds fTime):
OneToManyFilter()
{

	initializeEventMap();
	
	fType = VIDEO_SPLITTER;
	outputStreamInfo = new StreamInfo(VIDEO);
    outputStreamInfo->video.codec = RAW;
    outputStreamInfo->video.pixelFormat = RGB24;
    setFrameTime(fTime);
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
	//int degreeCrop = 0;
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
		//degreeCrop = cropsConfig[it.first]->getDegree();

		if((xROI >= 0 || yROI >= 0 || widthROI > 0 || heightROI > 0) && xROI+widthROI <= vFrame->getWidth() && yROI+heightROI <= vFrame->getHeight()){
			vFrameDst = dynamic_cast<VideoFrame*>(it.second);
			cropsConfig[it.first]->getCrop()->data = vFrameDst->getDataBuf();
			vFrameDst->setLength(widthROI * heightROI);
    		vFrameDst->setSize(widthROI, heightROI);

    		//Sin degree
    		orgFrame(cv::Rect(xROI, yROI, widthROI, heightROI)).copyTo(cropsConfig[it.first]->getCropRect(0, 0, widthROI, heightROI));
    		
    		//Con degree
    		/*if (degreeCrop == 0){
    			orgFrame(cv::Rect(xROI, yROI, widthROI, heightROI)).copyTo(cropsConfig[it.first]->getCropRect(0, 0, widthROI, heightROI));
			} else {
				cv::Mat rotationMatrix, rotatedImage;
				cv::Point orgFrameCenter(orgFrame.cols/2, orgFrame.rows/2);
				rotationMatrix = cv::getRotationMatrix2D(orgFrameCenter, degreeCrop, 1.0);
				cv::Rect bbox = cv::RotatedRect(orgFrameCenter,orgFrame.size(), degreeCrop).boundingRect();
				rotationMatrix.at<double>(0,2) += bbox.width/2.0 - orgFrameCenter.x;
    			rotationMatrix.at<double>(1,2) += bbox.height/2.0 - orgFrameCenter.y;

    			float limit = rotationMatrix.at<double>(0,2);
				float limitWidth = bbox.width - limit;
				float limitHeight = bbox.height - limit;
				if(xROI+yROI < limit || ((yROI + heightROI - limit) / (xROI)) > ((bbox.height - limit) / (limitWidth))
					|| (yROI + heightROI - bbox.height) / (xROI + widthROI - limitWidth) > (limitHeight - bbox.height) / (bbox.width - limitWidth)
					|| yROI / (xROI + widthROI - limit) < limitHeight / (bbox.width - limit)
				){
					utils::warningMsg("[VideoSplitter] Crop out of scope");
				} else {
					cv::warpAffine(orgFrame, rotatedImage, rotationMatrix, bbox.size());
					rotatedImage(cv::Rect(xROI, yROI, widthROI, heightROI)).copyTo(cropsConfig[it.first]->getCropRect(0, 0, widthROI, heightROI));
				}
				
			}*/
			it.second->setConsumed(true);
			it.second->setPresentationTime(org->getPresentationTime());
			it.second->setOriginTime(org->getOriginTime());
    		it.second->setSequenceNumber(org->getSequenceNumber());
			processFrame = true;
		} else {
			utils::warningMsg("[VideoSplitter] Crop not configured or out of scope (Crop ID: " + std::to_string(it.first) 
							+ " - Origin width: " + std::to_string(vFrame->getWidth()) + " - Origin height: " + std::to_string(vFrame->getHeight()) + ")");

			it.second->setConsumed(false);
		}
	}

	return processFrame;
}

void VideoSplitter::doGetState(Jzon::Object &filterNode)
{
	Jzon::Array jsonCropsConfigs;

	for(auto it : cropsConfig){
		Jzon::Object crConfig;
		crConfig.Add("id", it.first);
		crConfig.Add("width", it.second->getWidth());
        crConfig.Add("height", it.second->getHeight());
        crConfig.Add("x", it.second->getX());
        crConfig.Add("y", it.second->getY());
        crConfig.Add("degree", it.second->getDegree());
		jsonCropsConfigs.Add(crConfig);
	}
	filterNode.Add("frameTime", getConfigure());
	filterNode.Add("crops", jsonCropsConfigs);
}

bool VideoSplitter::configCrop0(int id, int width, int height, int x, int y, int degree)
{
	if (cropsConfig.count(id) <= 0) {
        utils::errorMsg("[VideoSplitter] Error configuring crop. Incorrect Id " + std::to_string(id));
        return false;
    }


    if (x < 0 || y < 0 || width <= 0 || height <= 0 || degree > 360 || degree < (-360)) {
        utils::errorMsg("[VideoSplitter] Error configuring crop. Incoherent values");
        return false;
    }

    cropsConfig[id]->config(width, height, x, y, degree);
    return true;
}

bool VideoSplitter::configure0(std::chrono::microseconds fTime)
{
	if (fTime.count() < 0) {
        utils::errorMsg("[VideoSplitter::configCrop0] Error, negative frame time is not valid");
        return NULL;
    }
    
    setFrameTime(fTime);

	return true;
}

void VideoSplitter::initializeEventMap()
{
	eventMap["configCrop"] = std::bind(&VideoSplitter::configCropEvent, this, std::placeholders::_1);
	eventMap["configure"] = std::bind(&VideoSplitter::configureEvent, this, std::placeholders::_1);
}

bool VideoSplitter::configCropEvent(Jzon::Node* params)
{
	if (!params) {
        utils::errorMsg("[VideoSplitter::configCropEvent] Params node missing");
        return false;
    }

    if (!params->Has("id") || !params->Has("width") || !params->Has("height") ||
            !params->Has("x") || !params->Has("y") || !params->Get("id").IsNumber() || 
            !params->Get("width").IsNumber() || !params->Get("height").IsNumber() || 
            !params->Get("x").IsNumber() || !params->Get("y").IsNumber()) {

        utils::errorMsg("[VideoSplitter::configCropEvent] Params node not complete");
        return false;
    }
    
    int id = params->Get("id").ToInt();
    int width = params->Get("width").ToInt();
    int height = params->Get("height").ToInt();
    int x = params->Get("x").ToInt();
    int y = params->Get("y").ToInt();
    int degree = 0;
    if(params->Has("degree")){
    	degree = params->Get("degree").ToInt();
    }
    utils::infoMsg("ID: " + std::to_string(id) + " W: " + std::to_string(width) + " H: " + std::to_string(height) + " X: " + std::to_string(x) + " Y: " + std::to_string(y)+ " Degree: " + std::to_string(degree));
    
    return configCrop0(id, width, height, x, y, degree);
}

bool VideoSplitter::configureEvent(Jzon::Node* params)
{
	if (!params) {
        utils::errorMsg("[VideoSplitter::configureEvent] Params node missing");
        return false;
    }

    std::chrono::microseconds fTime = std::chrono::microseconds(0);

    if (params->Has("fTime") && params->Get("fTime").IsNumber()){
  		fTime = std::chrono::microseconds(params->Get("fTime").ToInt());
    }

    return configure0(fTime);
}
        
bool VideoSplitter::specificWriterConfig(int writerID)
{
	if (cropsConfig.count(writerID) <= 0) {
        cropsConfig[writerID] = new CropConfig();
		return true;
    }
	
	utils::errorMsg("[VideoSplitter::specificWriterConfig] Error configuring. This WriterID exist " + std::to_string(writerID));
    return false;
}

bool VideoSplitter::specificWriterDelete(int writerID)
{
	if (cropsConfig.count(writerID) <= 0) {
        utils::errorMsg("[VideoSplitter::specificWriterDelete] Error configuring. This WriterID doesn't exist " + std::to_string(writerID));
        return false;
    }

	delete cropsConfig[writerID];
	cropsConfig.erase(writerID);

	return true;
}