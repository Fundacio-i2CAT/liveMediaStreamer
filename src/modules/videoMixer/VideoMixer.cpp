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

PositionSize::PositionSize(int width, int height, int x, int y, int layer)
{
    this->width = width;
    this->height = height;
    this->x = x;
    this->y = y;
}

VideoMixer::VideoMixer(int inputChannels)
{
    outputWidth = DEFAULT_WIDTH;
    outputHeight = DEFAULT_HEIGHT;

    PositionSize posSize(outputWidth, outputHeight, 0, 0);

    for (auto id : getAvailableReaders()) {
        positionAndSizes.insert(auto(id, posSize));
    }
}

VideoMixer::VideoMixer(int inputChannels, int outputWidth, int outputHeight)
{
    this->outputWidth = outputWidth;
    this->outputHeight = outputHeight;

    PositionSize posSize(outputWidth, outputHeight, 0, 0);

    for (auto id : getAvailableReaders()) {
        positionAndSizes.insert(auto(id, posSize));
    }
}

FrameQueue *allocQueue();

bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst)
{
    int frameNumber = orgFrames.size();
    VideoFrame *vFrame;

    layoutImg.data = dst->getDataBuf();

    for (int lay=0; lay < MAX_LAYERS; lay++) {
        for (auto frame : orgFrames) {
            vFrame = dynamic_cast<VideoFrame*>(frame.second);
            if (vFrame->getLayer() == lay) {
                pasteToLayout(frame.first, vFrame);
                frameNumber--;
            }

            if (frameNumber == 0) {
                break;
            }
        }

        if (frameNumber == 0) {
            break;
        }
    }

    return true;
}

void pasteToLayout(int frameID, VideoFrame* vFrame)
{
    Mat img(vFrame->getHeight(), vFrame->getWidth(), CV_8UC3, vFrame->getDataBuf());
    Size sz(positionAndSizes[frameID].width, positionAndSizes[frameID].height);
    int x = positionAndSizes[frameID].x;
    int y = positionAndSizes[frameID].y;

    if (img.size() != sz) {
        resize(img, layoutImg(Rect(x, y, sz.width, sz.height)), sz, 0, 0, INTER_LINEAR);
        return;
    }

    img.copyTo(layoutImg(Rect(x, y, sz.width, sz.height)));
}