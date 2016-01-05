/*
 *  VideoSplitter.hh - Class to handle crops of video 
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

#ifndef _VIDEO_SPLITTER_HH
#define _VIDEO_SPLITTER_HH

#include "../../VideoFrame.hh"
#include "../../Filter.hh"
#include "../../StreamInfo.hh"
#include <opencv/cv.hpp>



class CropConfig {
	public:
		/**
	    * Class constructor.
	    */
		CropConfig();
		
		/**
	    * It sets class attributes.
	    * @param width Channel
	    * @param height Channel
	    * @param x Upper left corner X position
	    * @param y Upper left corner Y position
	    * @param degree [-360º,360º] rotate image
	    */
		void config(int width, int height, int x, int y, int degree = 0);


		int getWidth() {return width;};
	    int getHeight() {return height;};
	    int getX() {return x;};
	    int getY() {return y;};
	    int getDegree() {return degree;};
	    cv::Mat *getCrop() {return &crop;};
	    cv::Mat getCropRect(int x, int y, int w, int h) { return crop(cv::Rect(x,y,w,h)); };
	private:
		int width;
	    int height;
	    int x;
	    int y;
	    int degree;
	    cv::Mat crop;
};

/*
* 	Video Splitter
*/

class VideoSplitter : public OneToManyFilter {
	public:
		/**
        * Class constructor
        * @param outputChannels 
        * @param fTime Frame time in microseconds
        * @return Pointer to new object if succeed of NULL if not
        */
		static VideoSplitter* createNew(std::chrono::microseconds fTime = std::chrono::microseconds(0));
		/**
        * Class destructor
        */
        ~VideoSplitter();

        /**
        * Configure Crop, validating introduced data
        * @param id Channel id
        * @param width See CropConfig::config
        * @param height See CropConfig::config
        * @param x See CropConfig::config
        * @param y See CropConfig::config
        * @param degree See CropConfig::config
        */
    	bool configCrop(int id, int width, int height, int x, int y, int degree=0);
    	bool configure(int fTime);
    	int getConfigure(){return getFrameTime().count();};

	protected:
		VideoSplitter(std::chrono::microseconds fTime);
		FrameQueue *allocQueue(ConnectionData cData);
		bool doProcessFrame(Frame *org, std::map<int, Frame *> &dstFrames);
		void doGetState(Jzon::Object &filterNode);
		bool configCrop0(int id, int width, int height, int x, int y, int degree=0);
		bool configure0(std::chrono::microseconds fTime);
		bool specificWriterConfig(int writerID);
        bool specificWriterDelete(int writerID);

	private:
		void initializeEventMap();
        bool configCropEvent(Jzon::Node* params);
        bool configureEvent(Jzon::Node* params);
        
        //There is no need of specific reader configuration
        bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
        bool specificReaderDelete(int /*readerID*/) {return true;};


        StreamInfo *outputStreamInfo;
        std::map<int, CropConfig*> cropsConfig;
};

#endif