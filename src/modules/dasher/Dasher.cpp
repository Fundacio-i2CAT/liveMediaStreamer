/*
 *  Dasher.cpp - Class that handles DASH sessions
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
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
 *            
 */
 
#include "Dasher.hh"
#include "../../AVFramedQueue.hh"
#include "DashVideoSegmenter.hh"
#include "DashAudioSegmenter.hh"

#include <map>
#include <string>
#include <chrono>
#include <fstream>

Dasher::Dasher(int readersNum) : TailFilter(readersNum)
{
}

Dasher::~Dasher()
{

}

bool Dasher::doProcessFrame(std::map<int, Frame*> orgFrames)
{
    for (auto fr : orgFrames) {

        if (!fr.second) {
            continue;
        }

        segmenters[fr.first]->manageFrame(fr.second);
    }

    return true;
}

void Dasher::initializeEventMap()
{

}

void Dasher::doGetState(Jzon::Object &filterNode)
{
//TODO: implement    
}

bool Dasher::addSegmenter(int readerId, std::string segBaseName, int segDurInMicroSeconds)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Reader* r;

    r = getReader(readerId);

    if (!r) {
        utils::errorMsg("Error adding segmenter: reader does not exist");
        return false;
    }

    if ((vQueue = dynamic_cast<VideoFrameQueue*>(r->getQueue())) != NULL) {

        if (vQueue->getCodec() != H264) {
            utils::errorMsg("Error setting dasher reader: only H264 codec is supported for video");
            return false;
        }

        segmenters[readerId] = new DashVideoSegmenter(segDurInMicroSeconds, segBaseName);
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return false;
        }

        segmenters[readerId] = new DashAudioSegmenter(segDurInMicroSeconds, segBaseName);
    }
    
    return true;
}

bool Dasher::removeSegmenter(int readerId)
{
    if (segmenters.count(readerId) <= 0) {
        utils::errorMsg("Error removing DASH segmenter: no segmenter associated to provided reader");
        return false;
    }

    segmenters[readerId]->finishSegment();

    delete segmenters[readerId];
    segmenters.erase(readerId);
    
    return true;
}

DashSegmenter::DashSegmenter(size_t segDur, size_t tBase, std::string segBaseName) : 
dashContext(NULL), timeBase(tBase), segmentDuration(segDur), frameDuration(0), baseName(segBaseName)
{
    segment = new DashSegment(MAX_DAT);
    initSegment = new DashSegment(MAX_DAT);
}

DashSegmenter::~DashSegmenter()
{
    delete segment;
    delete initSegment;
}

DashSegment::DashSegment(size_t maxSize)
{
    data = new unsigned char[maxSize];
    timestamp = 0;
    dataLength = 0;
    seqNumber = 0;
}

DashSegment::~DashSegment()
{
    delete[] data;
}

void DashSegment::setSeqNumber(size_t seqNum)
{
    seqNumber = seqNum;
}

void DashSegment::setDataLength(size_t length)
{
    dataLength = length;
}

bool DashSegment::writeToDisk(std::string path)
{
    const char* p = path.c_str();
    std::ofstream file(p, std::ofstream::binary);

    if (!file) {
        return false;
    }

    file.write((char*)data, dataLength);
    file.close();
    return true;
}

void DashSegment::setTimestamp(size_t ts)
{
    timestamp = ts;
}

void DashSegment::clear()
{
    timestamp = 0;
    dataLength = 0;
    seqNumber = 0;
}

