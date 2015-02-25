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

Dasher::Dasher(FilterRole role, bool sharedFrames, int readersNum) : TailFilter(role, sharedFrames, readersNum)
{
}

Dasher::~Dasher()
{

}

bool Dasher::doProcessFrame(std::map<int, Frame*> orgFrames)
{
    DashSegmenter* segmenter;
    bool newFrame;

    for (auto fr : orgFrames) {

        if (!fr.second) {
            continue;
        }

        segmenter = segmenters[fr.first];
        if (!segmenter->manageFrame(fr.second, newFrame)) {
            utils::errorMsg("Error managing frame");
            continue;
        }

        if (!newFrame ) {
            continue;
        }

        if (!segmenter->updateConfig()) {
            utils::errorMsg("[DashSegmenter] Error updating config");
            continue;
        }

        if (segmenter->generateInitSegment()) {
            utils::debugMsg("New DASH init segment generated");
        }

        if (segmenter->generateSegment()) {
            utils::debugMsg("New DASH segment generated");
        }
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

    if (segBaseName.empty() || segDurInMicroSeconds == 0) {
        utils::errorMsg("Error adding segmenter: empty segment base or segment duration 0");
        return false;
    }

    r = getReader(readerId);

    if (!r) {
        utils::errorMsg("Error adding segmenter: reader does not exist");
        return false;
    }

    if (segmenters.count(readerId) > 0) {
        utils::errorMsg("Error adding segmenter: there is a segmenter already assigned to this reader");
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

    if (!segmenters[readerId]->finishSegment()) {
        utils::errorMsg("Error removing DASH segmenter: last segment could not be written to disk. Anyway, segmenter has been deleted.");
        return false;
    }

    delete segmenters[readerId];
    segmenters.erase(readerId);

    return true;
}

DashSegmenter::DashSegmenter(size_t segDur, size_t tBase, std::string segBaseName, std::string segExt) :
dashContext(NULL), timeBase(tBase), segmentDuration(segDur), frameDuration(0), baseName(segBaseName),
segmentExt(segExt), tsOffset(0)
{
    segment = new DashSegment(MAX_DAT);
    initSegment = new DashSegment(MAX_DAT);
}

DashSegmenter::~DashSegmenter()
{
    delete segment;
    delete initSegment;
}

bool DashSegmenter::generateInitSegment()
{
    if (!updateMetadata()) {
        return false;
    }

    if (!generateInitData()) {
        utils::errorMsg("Error generating video init segment");
        return false;
    }

    if(!initSegment->writeToDisk(getInitSegmentName())) {
        utils::errorMsg("Error writing DASH init segment to disk: invalid path");
        return false;
    }

    return true;
}

bool DashSegmenter::generateSegment()
{
    if (!appendFrameToDashSegment()) {
        return false;
    }

    if(!segment->writeToDisk(getSegmentName())) {
        utils::errorMsg("Error writing DASH segment to disk: invalid path");
        return false;
    }

    segment->incrSeqNumber();

    return true;
}

std::string DashSegmenter::getInitSegmentName()
{
    std::string fullName;

    fullName = baseName + "_init" + segmentExt;

    return fullName;
}

std::string DashSegmenter::getSegmentName()
{
    std::string fullName;

    fullName = baseName + "_" + std::to_string(segment->getTimestamp()) + segmentExt;

    return fullName;
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
