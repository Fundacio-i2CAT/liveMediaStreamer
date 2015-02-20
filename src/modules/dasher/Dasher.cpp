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
#include <math.h>

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

Dasher::Dasher(std::string dashFolder, std::string baseName_, size_t segDurInSec, std::string mpdLocation, int readersNum) : 
TailFilter(readersNum)
{
    basePath = dashFolder;
    baseName = baseName_;
    mpdPath = basePath + baseName + ".mpd";
    segmentsBasePath = basePath + baseName;
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

        if (!generateInitSegment(fr.first, segmenter)) {
            utils::errorMsg("[DashSegmenter] Error generating init segment");
            continue;
        }

        if (generateSegment(fr.first, segmenter)) {
            utils::debugMsg("[DashSegmenter] New segment generated");
        }
    }

    return true;
}

bool Dasher::generateInitSegment(size_t id, DashSegmenter* segmenter)
{
    DashVideoSegmenter* vSeg;
    DashAudioSegmenter* aSeg;

    if ((vSeg = dynamic_cast<DashVideoSegmenter*>(segmenter)) != NULL) {
        
        if (!vSeg->generateInitSegment(initSegments[id])) {
            return true;
        }

        if(!initSegments[id]->writeToDisk(getInitSegmentName(basePath, baseName, id, V_EXT))) {
            utils::errorMsg("Error writing DASH segment to disk: invalid path");
            return false;
        }
    }

    if ((aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter)) != NULL) {

        if (!aSeg->generateInitSegment(initSegments[id])) {
            return true;
        }

        if(!initSegments[id]->writeToDisk(getInitSegmentName(basePath, baseName, id, A_EXT))) {
            utils::errorMsg("Error writing DASH segment to disk: invalid path");
            return false;
        }
    }

    if (!vSeg && !aSeg) {
        return false;
    }

    return true;
}

bool Dasher::generateSegment(size_t id, DashSegmenter* segmenter)
{
    DashVideoSegmenter* vSeg;
    DashAudioSegmenter* aSeg;
    size_t refTimestamp;
    size_t rmTimestamp;

    if ((vSeg = dynamic_cast<DashVideoSegmenter*>(segmenter)) != NULL) {
        
        if (!vSeg->generateSegment(vSegments[id])) {
            return false;
        }

        refTimestamp = updateTimestampControl(vSegments);
        mpdMngr->updateVideoAdaptationSet(V_ADAPT_SET_ID, segmenters[id]->getTimeBase(), vSegTempl, vInitSegTempl);
        mpdMngr->updateVideoRepresentation(V_ADAPT_SET_ID, std::to_string(id), VIDEO_CODEC, vSeg->getWidth(), 
                                            vSeg->getHeight(), V_BAND, vSeg->getFramerate());

        if (refTimestamp <= 0) {
            return false;
        }

        if (!writeSegmentsToDisk(vSegments, refTimestamp, V_EXT)) {
            utils::errorMsg("Error writing DASH video segment to disk");
            return false;
        }

        rmTimestamp = mpdMngr->updateAdaptationSetTimestamp(V_ADAPT_SET_ID, refTimestamp, segmenter->getSegDurInTimeBaseUnits());

        mpdMngr->writeToDisk(mpdPath.c_str());

        if (rmTimestamp > 0 && !cleanSegments(vSegments, rmTimestamp, V_EXT)) {
            utils::warningMsg("Error cleaning dash video segments");
        } 
    }

    if ((aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter)) != NULL) {

        if (!aSeg->generateSegment(aSegments[id])) {
            return false;
        }

        refTimestamp = updateTimestampControl(aSegments);
        mpdMngr->updateAudioAdaptationSet(A_ADAPT_SET_ID, segmenters[id]->getTimeBase(), aSegTempl, aInitSegTempl);
        mpdMngr->updateAudioRepresentation(A_ADAPT_SET_ID, std::to_string(id), AUDIO_CODEC, aSeg->getSampleRate(), A_BAND, aSeg->getChannels());

        if (refTimestamp <= 0) {
            return false;
        }

        if (!writeSegmentsToDisk(aSegments, refTimestamp, A_EXT)) {
            utils::errorMsg("Error writing DASH audio segment to disk");
            return false;
        }

        rmTimestamp = mpdMngr->updateAdaptationSetTimestamp(A_ADAPT_SET_ID, refTimestamp, segmenter->getSegDurInTimeBaseUnits());

        mpdMngr->writeToDisk(mpdPath.c_str());

        if (rmTimestamp > 0 && !cleanSegments(aSegments, rmTimestamp, A_EXT)) {
            utils::warningMsg("Error cleaning dash audio segments");
        } 
    }

    if (!vSeg && !aSeg) {
        return false;
    }

    return true;
}

size_t Dasher::updateTimestampControl(std::map<int,DashSegment*> segments)
{   
    size_t refTimestamp = 0;

    for (auto seg : segments) {

        if (seg.second->getTimestamp() <= 0) {
            return 0;
        }

        if (refTimestamp == 0) {
            refTimestamp = seg.second->getTimestamp();
        }


        if (refTimestamp != seg.second->getTimestamp()) {
            utils::warningMsg("Segments of the same Adaptation Set have different timestamps"); 
            utils::warningMsg("Setting timestamp to a reference one: this may cause playing errors");
        }
    }

    return refTimestamp;
}

bool Dasher::writeSegmentsToDisk(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt)
{
    for (auto seg : segments) {

        if(!seg.second->writeToDisk(getSegmentName(basePath, baseName, seg.first, timestamp, segExt))) {
            utils::errorMsg("Error writing DASH segment to disk: invalid path");
            return false;
        }

        seg.second->clear();
        seg.second->incrSeqNumber();
    }

    return true;
}

bool Dasher::cleanSegments(std::map<int,DashSegment*> segments, size_t timestamp, std::string segExt)
{
    bool success = true;
    std::string segmentName;

    for (auto seg : segments) {
        segmentName = getSegmentName(basePath, baseName, seg.first, timestamp, segExt);

        if (std::remove(segmentName.c_str()) != 0) {
            success &= false;
            utils::warningMsg("Error cleaning dash segment: " + segmentName);
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

        segmenters[readerId] = new DashVideoSegmenter(segDurInMicrosec);
        segmenters[readerId]->setOffset(timestampOffset);
        vSegments[readerId] = new DashSegment();
        initSegments[readerId] = new DashSegment();
    }
    
    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return false;
        }

        segmenters[readerId] = new DashAudioSegmenter(segDurInMicrosec);
        segmenters[readerId]->setOffset(timestampOffset);
        aSegments[readerId] = new DashSegment();
        initSegments[readerId] = new DashSegment();
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

std::string Dasher::getSegmentName(std::string basePath, std::string baseName, size_t reprId, size_t timestamp, std::string ext)
{
    std::string fullName;
    fullName = basePath + baseName + "_" + std::to_string(reprId) + "_" + std::to_string(timestamp) + ext;

    return fullName;
}

std::string Dasher::getInitSegmentName(std::string basePath, std::string baseName, size_t reprId, std::string ext)
{
    std::string fullName;
    fullName = basePath + baseName + "_" + std::to_string(reprId) + "_init" + ext;

    return fullName;
}


DashSegmenter::DashSegmenter(size_t segDurInMicros, size_t tBase) : 
dashContext(NULL), timeBase(tBase), segDurInMicroSec(segDurInMicros), frameDuration(0), tsOffset(0), theoricPts(0)
{
    segDurInTimeBaseUnits = segDurInMicroSec*timeBase/MICROSECONDS_TIME_BASE;
}

DashSegmenter::~DashSegmenter()
{

}

bool DashSegmenter::generateInitSegment(DashSegment* segment) 
{
    if (!updateMetadata()) {
        return false;
    }

    if (!generateInitData(segment)) {
        utils::errorMsg("Error generating video init segment");
        return false;
    }

    return true;
}

bool DashSegmenter::generateSegment(DashSegment* segment)
{
    if (!appendFrameToDashSegment(segment)) {
        return false;
    }

    return true;
}

void DashSegmenter::setOffset(size_t offs)
{
    tsOffset = offs;
}

size_t DashSegmenter::customTimestamp(size_t timestamp)
{
    return (timestamp - tsOffset)*timeBase/MICROSECONDS_TIME_BASE;
}

size_t DashSegmenter::microsToTimeBase(size_t microsValue)
{
    double result;
    size_t roundedResult;

    result = (double)microsValue*timeBase/MICROSECONDS_TIME_BASE;
    roundedResult = round(result);

    return roundedResult;
}


/////////////////
// DashSegment //
/////////////////

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
}

