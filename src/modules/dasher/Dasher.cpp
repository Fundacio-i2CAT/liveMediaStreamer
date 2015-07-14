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
 *            Gerard Castillo <gerard.castillo@i2cat.net>
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

Dasher* Dasher::createNew(std::string dashFolder, std::string baseName, size_t segDurInSeconds, int readersNum)
{
    Dasher* dasher = NULL;

    dasher = new Dasher(readersNum);

    if (!dasher->configure(dashFolder, baseName, segDurInSeconds)) {
        utils::errorMsg("Error configuring Dasher");
        delete dasher;
        return NULL;
    }

    return dasher;
}


Dasher::Dasher(int readersNum) :
TailFilter(MASTER, readersNum), mpdMngr(NULL), hasVideo(false), videoStarted(false)
{
    fType = DASHER;
    initializeEventMap();
}

Dasher::~Dasher()
{
    for (auto seg : segmenters) {
        delete seg.second;
    }
    delete mpdMngr;
}

bool Dasher::configure(std::string dashFolder, std::string baseName_, size_t segDurInSec)
{
    if (access(dashFolder.c_str(), W_OK) != 0) {
        utils::errorMsg("Error configuring Dasher: provided folder is not writable");
        return false;
    }

    if (dashFolder.back() != '/') {
        dashFolder.append("/");
    }

    if (baseName_.empty() || segDurInSec == 0) {
        utils::errorMsg("Error configuring Dasher: provided parameters are not valid");
        return false;
    }

    basePath = dashFolder;
    baseName = baseName_;
    mpdPath = basePath + baseName + ".mpd";
    vSegTempl = baseName + "_$RepresentationID$_$Time$.m4v";
    aSegTempl = baseName + "_$RepresentationID$_$Time$.m4a";
    vInitSegTempl = baseName + "_$RepresentationID$_init.m4v";
    aInitSegTempl = baseName + "_$RepresentationID$_init.m4a";

    mpdMngr = new MpdManager();
    mpdMngr->setMinBufferTime(segDurInSec*(MAX_SEGMENTS_IN_MPD/2));
    mpdMngr->setMinimumUpdatePeriod(segDurInSec);
    mpdMngr->setTimeShiftBufferDepth(segDurInSec*MAX_SEGMENTS_IN_MPD);

    timestampOffset = std::chrono::system_clock::now();
    segDur = std::chrono::seconds(segDurInSec);

    return true;
}

bool Dasher::doProcessFrame(std::map<int, Frame*> orgFrames)
{
    DashSegmenter* segmenter;
    bool newFrame;

    if (!mpdMngr) {
        utils::errorMsg("Dasher MUST be configured in order to process frames");
        return false;
    }

    for (auto fr : orgFrames) {

        if (!fr.second || !fr.second->getConsumed()) {
            continue;
        }

        segmenter = getSegmenter(fr.first);

        if (!segmenter) {
            continue;
        }

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

        if (!appendFrameToSegment(fr.first, segmenter)) {
            utils::errorMsg("[DashSegmenter] Error generating init segment");
            continue;
        }
    }

    return true;
}

bool Dasher::appendFrameToSegment(size_t id, DashSegmenter* segmenter)
{
    DashVideoSegmenter* vSeg;
    DashAudioSegmenter* aSeg;

    if ((vSeg = dynamic_cast<DashVideoSegmenter*>(segmenter)) != NULL) {

        if (!vSeg->appendFrameToDashSegment(vSegments[id])) {
            utils::errorMsg("Error appending video frame to segment");
            return false;
        }

        videoStarted = true;
    }   

    if ((aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter)) != NULL) {

        if (hasVideo && !videoStarted) {
            mpdMngr->flushAdaptationSetTimestamps(A_ADAPT_SET_ID);
            aSeg->flushDashContext();
            return true;
        }

        if (!aSeg->appendFrameToDashSegment(aSegments[id])) {
            utils::errorMsg("Error appending audio frame to segment");
            return false;
        }
    }

    if (!vSeg && !aSeg) {
        return false;
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
    size_t refDuration;
    size_t rmTimestamp;

    if ((vSeg = dynamic_cast<DashVideoSegmenter*>(segmenter)) != NULL) {

        if (!vSeg->generateSegment(vSegments[id])) {
            return false;
        }

        mpdMngr->updateVideoAdaptationSet(V_ADAPT_SET_ID, segmenters[id]->getTimeBase(), vSegTempl, vInitSegTempl);
        mpdMngr->updateVideoRepresentation(V_ADAPT_SET_ID, std::to_string(id), VIDEO_CODEC, vSeg->getWidth(),
                                            vSeg->getHeight(), vSeg->getBitrate(), vSeg->getFramerate());

        if (!updateTimestampControl(vSegments, refTimestamp, refDuration)) {
            return false;
        }

        if (!forceAudioSegmentsGeneration()) {
            utils::errorMsg("Error forcing the generation of audio segments. This may cause errors!");
        }

        if (!writeSegmentsToDisk(vSegments, refTimestamp, V_EXT)) {
            utils::errorMsg("Error writing DASH video segment to disk");
            return false;
        }

        rmTimestamp = mpdMngr->updateAdaptationSetTimestamp(V_ADAPT_SET_ID, refTimestamp, refDuration);

        mpdMngr->writeToDisk(mpdPath.c_str());

        if (rmTimestamp > 0 && !cleanSegments(vSegments, rmTimestamp, V_EXT)) {
            utils::warningMsg("Error cleaning dash video segments");
        }
    }

    if ((aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter)) != NULL) {

        if (!aSeg->generateSegment(aSegments[id])) {
            return false;
        }

        mpdMngr->updateAudioAdaptationSet(A_ADAPT_SET_ID, segmenters[id]->getTimeBase(), aSegTempl, aInitSegTempl);
        mpdMngr->updateAudioRepresentation(A_ADAPT_SET_ID, std::to_string(id), AUDIO_CODEC, 
                                            aSeg->getSampleRate(), aSeg->getBitrate(), aSeg->getChannels());

        if (!updateTimestampControl(aSegments, refTimestamp, refDuration)) {
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

bool Dasher::forceAudioSegmentsGeneration()
{
    DashSegmenter* segmenter;
    DashAudioSegmenter* aSeg;
    size_t refTimestamp;
    size_t refDuration;
    size_t rmTimestamp;

    for (auto seg : aSegments) {

        segmenter = getSegmenter(seg.first);

        if (!segmenter) {
            continue;
        }

        aSeg = dynamic_cast<DashAudioSegmenter*>(segmenter);

        if (!aSeg) {
            continue;
        }

        if (!aSeg->forceGenerateSegment(seg.second)) {
            utils::errorMsg("Error forcing audio segment generation");
            return false;
        }

        mpdMngr->updateAudioAdaptationSet(A_ADAPT_SET_ID, segmenters[seg.first]->getTimeBase(), aSegTempl, aInitSegTempl);
        mpdMngr->updateAudioRepresentation(A_ADAPT_SET_ID, std::to_string(seg.first), AUDIO_CODEC, 
                                            aSeg->getSampleRate(), aSeg->getBitrate(), aSeg->getChannels());

         if (!updateTimestampControl(aSegments, refTimestamp, refDuration)) {
            continue;
        }

        if (!writeSegmentsToDisk(aSegments, refTimestamp, A_EXT)) {
            utils::errorMsg("Error writing DASH audio segment to disk");
            return false;
        }

        rmTimestamp = mpdMngr->updateAdaptationSetTimestamp(A_ADAPT_SET_ID, refTimestamp, refDuration);

        if (rmTimestamp > 0 && !cleanSegments(aSegments, rmTimestamp, A_EXT)) {
            utils::warningMsg("Error cleaning dash audio segments");
        }
    }

    return true;
}

bool Dasher::updateTimestampControl(std::map<int,DashSegment*> segments, size_t &timestamp, size_t &duration)
{
    size_t refTimestamp = 0;
    size_t refDuration = 0;

    for (auto seg : segments) {

        if (seg.second->getTimestamp() <= 0 || seg.second->getTimestamp() <= 0) {
            return false;
        }

        if (refTimestamp == 0 && refDuration == 0) {
            refTimestamp = seg.second->getTimestamp();
            refDuration = seg.second->getDuration();
        }

        if (refTimestamp != seg.second->getTimestamp() || refDuration != seg.second->getDuration()) {
            utils::warningMsg("Segments of the same Adaptation Set have different timestamps and/or durations");
            utils::warningMsg("Setting timestamp and/or duration to a reference one: this may cause playing errors");
        }
    }

    if (refTimestamp == 0 || refDuration == 0) {
        return false;
    }

    timestamp = refTimestamp;
    duration = refDuration;
    return true;
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

    return success;
}


void Dasher::initializeEventMap()
{
    eventMap["configure"] = std::bind(&Dasher::configureEvent, this, std::placeholders::_1);
    eventMap["addSegmenter"] = std::bind(&Dasher::addSegmenterEvent, this, std::placeholders::_1);
    eventMap["removeSegmenter"] = std::bind(&Dasher::removeSegmenterEvent, this, std::placeholders::_1);
    eventMap["setBitrate"] = std::bind(&Dasher::setBitrateEvent, this, std::placeholders::_1);
}

void Dasher::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("folder", basePath);
    filterNode.Add("baseName", baseName);
    filterNode.Add("mpdURI", mpdPath);
    filterNode.Add("segDurInSec", std::to_string(segDur.count()));
}

bool Dasher::configureEvent(Jzon::Node* params)
{
    std::string dashFolder = basePath;
    std::string bName = baseName;
    size_t segDurInSec = segDur.count();

    if (!params) {
        return false;
    }

    if (params->Has("folder")) {
        dashFolder = params->Get("folder").ToString();
    }

    if (params->Has("baseName")) {
        baseName = params->Get("baseName").ToString();
    }

    if (params->Has("segDurInSec")) {
        segDurInSec = params->Get("segDurInSec").ToInt();
    }

    return configure(dashFolder, baseName, segDurInSec);
}

bool Dasher::addSegmenterEvent(Jzon::Node* params)
{
    int id;

    if (!params) {
        return false;
    }

    if (!params->Has("id")){
        return false;
    }

    id = params->Get("id").ToInt();

    return addSegmenter(id);
}

bool Dasher::removeSegmenterEvent(Jzon::Node* params)
{
    int id;

    if (!params) {
        return false;
    }

    if (!params->Has("id")){
        return false;
    }

    id = params->Get("id").ToInt();

    return removeSegmenter(id);
}

bool Dasher::setBitrateEvent(Jzon::Node* params)
{
    int id, bitrate;

    if (!params) {
        return false;
    }

    if (!params->Has("id") || !params->Has("bitrate")) {
        return false;
    }

    id = params->Get("id").ToInt();
    bitrate = params->Get("bitrate").ToInt();

    return setDashSegmenterBitrate(id, bitrate);
}

bool Dasher::addSegmenter(int readerId)
{
    VideoFrameQueue *vQueue;
    AudioFrameQueue *aQueue;
    Reader* r;
    std::string completeSegBasePath;

    if (!mpdMngr) {
        utils::errorMsg("Dasher MUST be configured in order to add a mew segmenter");
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

        segmenters[readerId] = new DashVideoSegmenter(segDur);
        vSegments[readerId] = new DashSegment();
        initSegments[readerId] = new DashSegment();
        hasVideo = true;
    }

    if ((aQueue = dynamic_cast<AudioFrameQueue*>(r->getQueue())) != NULL) {

        if (aQueue->getCodec() != AAC) {
            utils::errorMsg("Error setting Dasher reader: only AAC codec is supported for audio");
            return false;
        }

        segmenters[readerId] = new DashAudioSegmenter(segDur);
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

    if (vSegments.count(readerId) > 0) {
        delete vSegments[readerId];
        vSegments.erase(readerId);
        mpdMngr->removeRepresentation(V_ADAPT_SET_ID, std::to_string(readerId));
    }

    if (aSegments.count(readerId) > 0) {
        delete aSegments[readerId];
        aSegments.erase(readerId);
        mpdMngr->removeRepresentation(A_ADAPT_SET_ID, std::to_string(readerId));
    }

    if (initSegments.count(readerId) > 0) {
        delete initSegments[readerId];
        initSegments.erase(readerId);
    }

    delete segmenters[readerId];
    segmenters.erase(readerId);

    if (vSegments.empty()) {
        hasVideo = false;
        videoStarted = false;
    }

    mpdMngr->writeToDisk(mpdPath.c_str());
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

DashSegmenter* Dasher::getSegmenter(size_t id)
{
    if (segmenters.count(id) <= 0) {
        return NULL;
    }

    return segmenters[id];
}

bool Dasher::setDashSegmenterBitrate(int id, size_t bps)
{
    DashSegmenter* segmenter;

    segmenter = getSegmenter(id);

    if (!segmenter) {
        utils::errorMsg("Error setting bitrate. Provided id does not exist");
        return false;
    }

    segmenter->setBitrate(bps);
    return true;
}

///////////////////
// DashSegmenter //
///////////////////

DashSegmenter::DashSegmenter(std::chrono::seconds segmentDuration, size_t tBase) :
segDur(segmentDuration), dashContext(NULL), timeBase(tBase), frameDuration(0), bitrateInBitsPerSec(0)
{
    segDurInTimeBaseUnits = segDur.count()*timeBase;
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

size_t DashSegmenter::nanosToTimeBase(std::chrono::nanoseconds nanosValue)
{
    return nanosValue.count()*timeBase/std::nano::den;
}

size_t DashSegmenter::microsToTimeBase(std::chrono::microseconds microValue)
{
    return microValue.count()*timeBase/std::micro::den;
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

void DashSegment::setDuration(size_t dur)
{
    duration = dur;
}

void DashSegment::clear()
{
    timestamp = 0;
    duration = 0;
    dataLength = 0;
}
