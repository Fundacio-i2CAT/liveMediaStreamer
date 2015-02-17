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
#include <unistd.h>

#define V_BAND 2000000
#define A_BAND 192000

Dasher* Dasher::createNew(std::string dashFolder, std::string baseName, size_t segDurInSeconds, std::string mpdLocation, int readersNum)
{
    std::string mpdPath;
    std::string segmentsBasePath;

    if (access(dashFolder.c_str(), W_OK) != 0) {
        utils::errorMsg("Error creating Dasher: provided folder is not writable");
        return NULL;
    }

    if (dashFolder.back() != '/') {
        dashFolder.append("/");
    }

    return new Dasher(dashFolder, baseName, segDurInSeconds, mpdLocation);
}

Dasher::Dasher(std::string dashFolder, std::string baseName, size_t segDurInSec, std::string mpdLocation, int readersNum) : 
TailFilter(readersNum)
{
    mpdPath = dashFolder + baseName + ".mpd";
    segmentsBasePath = dashFolder + baseName;
    vSegTempl = baseName + "_$RepresentationID$_$Time$.m4v";
    aSegTempl = baseName + "_$RepresentationID$_$Time$.m4a";
    vInitSegTempl = baseName + "_$RepresentationID$_init.m4v";
    aInitSegTempl = baseName + "_$RepresentationID$_init.m4a";

    mpdMngr = new MpdManager();
    mpdMngr->setLocation(mpdLocation);
    mpdMngr->setMinBufferTime(segDurInSec*(MAX_SEGMENTS_IN_MPD/2));
    mpdMngr->setMinimumUpdatePeriod(segDurInSec);
    mpdMngr->setTimeShiftBufferDepth(segDurInSec*MAX_SEGMENTS_IN_MPD);

    timestampOffset = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    segDurInMicrosec = segDurInSec*1000000;
}

Dasher::~Dasher()
{
    for (auto seg : segmenters) {
        delete seg.second;
    }
    delete mpdMngr;
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
            updateMpd(fr.first, segmenter);
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

bool Dasher::addSegmenter(int readerId)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Reader* r;
    std::string completeSegBasePath;

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

        completeSegBasePath = segmentsBasePath + "_" + std::to_string(readerId);
        segmenters[readerId] = new DashVideoSegmenter(segDurInMicrosec, completeSegBasePath);
        segmenters[readerId]->setOffset(timestampOffset);
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return false;
        }

        completeSegBasePath = segmentsBasePath + "_" + std::to_string(readerId);
        segmenters[readerId] = new DashAudioSegmenter(segDurInMicrosec, completeSegBasePath);
        segmenters[readerId]->setOffset(timestampOffset);
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

void Dasher::updateMpd(int id, DashSegmenter* segmenter)
{
    DashVideoSegmenter* vSeg;
    DashAudioSegmenter* aSeg;

    if ((vSeg = dynamic_cast<DashVideoSegmenter*>(segmenter)) != NULL) {
        mpdMngr->updateVideoAdaptationSet(V_ADAPT_SET_ID, segmenter->getTimeBase(), vSegTempl, vInitSegTempl);
        mpdMngr->updateVideoRepresentation(V_ADAPT_SET_ID, std::to_string(id), VIDEO_CODEC, vSeg->getWidth(), 
                                            vSeg->getHeight(), V_BAND, vSeg->getFramerate());
        mpdMngr->updateAdaptationSetTimestamp(V_ADAPT_SET_ID, segmenter->getSegmentTimestamp(), vSeg->getSegmentDuration());
    }

    if ((aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter)) != NULL) {
        mpdMngr->updateAudioAdaptationSet(A_ADAPT_SET_ID, segmenter->getTimeBase(), aSegTempl, aInitSegTempl);
        mpdMngr->updateAudioRepresentation(A_ADAPT_SET_ID, std::to_string(id), AUDIO_CODEC, aSeg->getSampleRate(), A_BAND, aSeg->getChannels());
        mpdMngr->updateAdaptationSetTimestamp(A_ADAPT_SET_ID, segmenter->getSegmentTimestamp(), aSeg->getSegmentDuration());
    }

    mpdMngr->writeToDisk(mpdPath.c_str());
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

size_t DashSegmenter::getSegmentTimestamp() 
{
    return segment->getTimestamp();
}

void DashSegmenter::setOffset(size_t offs)
{
    tsOffset = offs;
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

