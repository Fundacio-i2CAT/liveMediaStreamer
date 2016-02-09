/*
 *  DashVideoSegmenterTest.cpp - DashVideoSegmenter class test
 *  Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:    Marc Palau <marc.palau@i2cat.net>
 *              Gerard Castillo <gerard.castillo@i2cat.net>
 *
 */

#include <string>
#include <iostream>
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "modules/dasher/DashVideoSegmenter.hh"
#include "modules/dasher/DashVideoSegmenterAVC.hh"
#include "modules/dasher/DashVideoSegmenterHEVC.hh"

#define SEG_DURATION 2
#define BASE_NAME_AVC "testsData/modules/dasher/dashVideoSegmenterAVCTest/test"
#define BASE_NAME_HEVC "testsData/modules/dasher/dashVideoSegmenterHEVCTest/test"
#define WIDTH 1280
#define HEIGHT 534
#define FRAMERATE 25

size_t readFile(char const* fileName, char* dstBuffer)
{
    size_t inputDataSize;
    std::ifstream inputDataFile(fileName, std::ios::in|std::ios::binary|std::ios::ate);

    if (!inputDataFile.is_open()) {
        utils::errorMsg("Error opening file " + std::string(fileName));
        CPPUNIT_FAIL("Test data upload failed. Check test data file paths\n");
        return 0;
    }

    inputDataSize = inputDataFile.tellg();
    inputDataFile.seekg (0, std::ios::beg);
    inputDataFile.read(dstBuffer, inputDataSize);
    return inputDataSize;
}

/*
*   AVC Test
*/
class DashVideoSegmenterAVCTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DashVideoSegmenterAVCTest);
    CPPUNIT_TEST(manageInvalidFrames);
    CPPUNIT_TEST(manageNonVCLNals);
    CPPUNIT_TEST(manageIDRNals);
    CPPUNIT_TEST(manageNonIDRNals);
    CPPUNIT_TEST(generateInitSegment);
    CPPUNIT_TEST(appendFrameToDashSegment);
    CPPUNIT_TEST(generateSegment);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageInvalidFrames();
    void manageNonVCLNals();
    void manageIDRNals();
    void manageNonIDRNals();
    void generateInitSegment();
    void appendFrameToDashSegment();
    void generateSegment();

    DashVideoSegmenterAVC* segmenter;
    AudioFrame* aFrame;
    VideoFrame *spsNal;
    VideoFrame *ppsNal;
    VideoFrame *seiNal;
    VideoFrame *audNal;
    VideoFrame *idrNal;
    VideoFrame *nonIdrNal;
    VideoFrame *dummyNal;
    std::chrono::microseconds frameTime;
    std::chrono::microseconds timestamp = std::chrono::microseconds(0);
};

void DashVideoSegmenterAVCTest::setUp()
{
    size_t dataLength;

    frameTime = std::chrono::microseconds(40000);

    segmenter = new DashVideoSegmenterAVC(std::chrono::seconds(SEG_DURATION), timestamp);
    dummyNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);

    spsNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);
    ppsNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);
    seiNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);
    audNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);
    idrNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);
    nonIdrNal = InterleavedVideoFrame::createNew(H264, MAX_H264_OR_5_NAL_SIZE);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_sps", (char*)spsNal->getDataBuf());
    spsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_pps", (char*)ppsNal->getDataBuf());
    ppsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_sei", (char*)seiNal->getDataBuf());
    seiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_aud", (char*)audNal->getDataBuf());
    audNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_idr_0", (char*)idrNal->getDataBuf());
    idrNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_nonIdr0", (char*)nonIdrNal->getDataBuf());
    nonIdrNal->setLength(dataLength);
}

void DashVideoSegmenterAVCTest::tearDown()
{
    delete segmenter;
    delete spsNal;
    delete ppsNal;
    delete seiNal;
    delete audNal;
    delete idrNal;
    delete nonIdrNal;
    delete dummyNal;
}

void DashVideoSegmenterAVCTest::manageInvalidFrames()
{
    Frame* frame;

    frame = segmenter->manageFrame(aFrame);
    CPPUNIT_ASSERT(!frame);
    
    frame = segmenter->manageFrame(dummyNal);
    CPPUNIT_ASSERT(!frame);
}

void DashVideoSegmenterAVCTest::manageNonVCLNals()
{
    Frame* frame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);

    spsNal->setPresentationTime(timestamp);
    ppsNal->setPresentationTime(timestamp);
    seiNal->setPresentationTime(timestamp);

    frame = segmenter->manageFrame(spsNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(ppsNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(seiNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());
}

void DashVideoSegmenterAVCTest::manageIDRNals()
{
    Frame* frame;
    VideoFrame* vFrame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);
    int nalStartCodeLength = 3;
    int avccHeaderLength = 4;

    audNal->setPresentationTime(timestamp);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    idrNal->setPresentationTime(timestamp);
    idrNal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(idrNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    audNal->setPresentationTime(timestamp + frameTime);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(segmenter->isPreviousFrameIntra());

    vFrame = dynamic_cast<VideoFrame*>(frame);

    CPPUNIT_ASSERT(vFrame);
    CPPUNIT_ASSERT(vFrame->getWidth() == WIDTH);
    CPPUNIT_ASSERT(vFrame->getHeight() == HEIGHT);
    CPPUNIT_ASSERT(vFrame->getPresentationTime() == timestamp);

    CPPUNIT_ASSERT(vFrame->getLength() == idrNal->getLength() - nalStartCodeLength + avccHeaderLength);
}

void DashVideoSegmenterAVCTest::manageNonIDRNals()
{
    Frame* frame;
    VideoFrame* vFrame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);
    int nalStartCodeLength = 3;
    int avccHeaderLength = 4;

    audNal->setPresentationTime(timestamp);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    nonIdrNal->setPresentationTime(timestamp);
    nonIdrNal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(nonIdrNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    audNal->setPresentationTime(timestamp + frameTime);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    vFrame = dynamic_cast<VideoFrame*>(frame);

    CPPUNIT_ASSERT(vFrame);
    CPPUNIT_ASSERT(vFrame->getWidth() == WIDTH);
    CPPUNIT_ASSERT(vFrame->getHeight() == HEIGHT);
    CPPUNIT_ASSERT(vFrame->getPresentationTime() == timestamp);

    CPPUNIT_ASSERT(vFrame->getLength() == nonIdrNal->getLength() - nalStartCodeLength + avccHeaderLength);
}

void DashVideoSegmenterAVCTest::generateInitSegment()
{
    char* initModel = new char[MAX_DAT];
    size_t initModelLength;
    DashSegment* initSegment = new DashSegment();
    std::chrono::microseconds ts(1000);
    Frame* frame;

    spsNal->setPresentationTime(ts);
    ppsNal->setPresentationTime(ts);
    idrNal->setSize(WIDTH,HEIGHT);
    idrNal->setPresentationTime(ts);

    initModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/initModel.m4v", initModel);

    frame = segmenter->manageFrame(spsNal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(ppsNal);
    CPPUNIT_ASSERT(!frame);

    CPPUNIT_ASSERT(!segmenter->generateInitSegment(initSegment));

    frame = segmenter->manageFrame(idrNal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(segmenter->generateInitSegment(initSegment));

    CPPUNIT_ASSERT(initModelLength == initSegment->getDataLength());
    CPPUNIT_ASSERT(memcmp(initModel, initSegment->getDataBuffer(), initSegment->getDataLength()) == 0);
}

void DashVideoSegmenterAVCTest::appendFrameToDashSegment()
{
    Frame* frame = NULL;
    std::chrono::microseconds ts(1000);

    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(frame));

    idrNal->setPresentationTime(ts);
    idrNal->setSize(WIDTH, HEIGHT);
    audNal->setPresentationTime(ts);

    frame = segmenter->manageFrame(idrNal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(segmenter->appendFrameToDashSegment(frame));
}

void DashVideoSegmenterAVCTest::generateSegment()
{
    char* segmentModel = new char[MAX_DAT];
    size_t segmentModelLength;
    DashSegment* segment = new DashSegment();

    Frame* frame = NULL;
    std::chrono::microseconds ts(1000);
    size_t nalCounter = 0;
    size_t dataLength = 0;
    std::string strName;

    segmentModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterAVCTest/segmentModel.m4v", segmentModel);

    dummyNal->setSize(WIDTH, HEIGHT);
    CPPUNIT_ASSERT(!segmenter->generateSegment(segment, frame));

    while (true) {
        strName = "testsData/modules/dasher/dashVideoSegmenterAVCTest/nalModels/nal_" + std::to_string(nalCounter);
        dataLength = readFile(strName.c_str(), (char*)dummyNal->getDataBuf());
        dummyNal->setLength(dataLength);
        dummyNal->setPresentationTime(ts);
        frame = segmenter->manageFrame(dummyNal);
        nalCounter++;

        if (!frame) {
            continue;
        }

        ts += frameTime;

        if (segmenter->generateSegment(segment, frame)) {
            break;
        }

        if(!segmenter->appendFrameToDashSegment(frame)) {
            CPPUNIT_FAIL("Segmenter appendFrameToDashSegment failed when testing general workflow\n");
        }
    }

    CPPUNIT_ASSERT(segmentModelLength == segment->getDataLength());
}

/*
*   HEVC Test
*/
class DashVideoSegmenterHEVCTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DashVideoSegmenterHEVCTest);
    CPPUNIT_TEST(manageInvalidFrames);
    CPPUNIT_TEST(manageNonVCLNals);
    CPPUNIT_TEST(manageIDRNals);
    CPPUNIT_TEST(manageNonIDRNals);
    CPPUNIT_TEST(generateInitSegment);
    CPPUNIT_TEST(appendFrameToDashSegment);
    CPPUNIT_TEST(generateSegment);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageInvalidFrames();
    void manageNonVCLNals();
    void manageIDRNals();
    void manageNonIDRNals();
    void generateInitSegment();
    void appendFrameToDashSegment();
    void generateSegment();

    DashVideoSegmenterHEVC* segmenter;
    AudioFrame* aFrame;
    VideoFrame *vpsNal;
    VideoFrame *spsNal;
    VideoFrame *ppsNal;
    VideoFrame *prefixSeiNal;
    VideoFrame *suffixSeiNal;
    VideoFrame *audNal;
    VideoFrame *idr1Nal;
    VideoFrame *idr2Nal;
    VideoFrame *craNal;
    VideoFrame *nonTsaNal;
    VideoFrame *dummyNal;
    std::chrono::microseconds frameTime;
    
    std::chrono::microseconds timestamp = std::chrono::microseconds(0);
};

void DashVideoSegmenterHEVCTest::setUp()
{
    size_t dataLength;

    frameTime = std::chrono::microseconds(40000);

    segmenter = new DashVideoSegmenterHEVC(std::chrono::seconds(SEG_DURATION), timestamp);
    dummyNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    vpsNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    spsNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);
    ppsNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    prefixSeiNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);
    suffixSeiNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    audNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    idr1Nal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);
    idr2Nal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);
    craNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);
    nonTsaNal = InterleavedVideoFrame::createNew(H265, MAX_H264_OR_5_NAL_SIZE);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_vps_0", (char*)vpsNal->getDataBuf());
    vpsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_sps_0", (char*)spsNal->getDataBuf());
    spsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_pps_0", (char*)ppsNal->getDataBuf());
    ppsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_prefix_sei_nut_0", (char*)prefixSeiNal->getDataBuf());
    prefixSeiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_suffix_sei_nut_0", (char*)suffixSeiNal->getDataBuf());
    suffixSeiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_aud_0", (char*)audNal->getDataBuf());
    audNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_idr1_0", (char*)idr1Nal->getDataBuf());
    idr1Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_idr2_0", (char*)idr2Nal->getDataBuf());
    idr2Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_cra_0", (char*)craNal->getDataBuf());
    craNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_non_tsa_stsa_1_0", (char*)nonTsaNal->getDataBuf());
    nonTsaNal->setLength(dataLength);
}

void DashVideoSegmenterHEVCTest::tearDown()
{
    delete segmenter;
    delete vpsNal;
    delete spsNal;
    delete ppsNal;
    delete prefixSeiNal;
    delete suffixSeiNal;
    delete audNal;
    delete idr1Nal;
    delete idr2Nal;
    delete craNal;
    delete nonTsaNal;
    delete dummyNal;
}

void DashVideoSegmenterHEVCTest::manageInvalidFrames()
{
    Frame* frame;

    frame = segmenter->manageFrame(aFrame);
    CPPUNIT_ASSERT(!frame);
    
    frame = segmenter->manageFrame(dummyNal);
    CPPUNIT_ASSERT(!frame);
}

void DashVideoSegmenterHEVCTest::manageNonVCLNals()
{
    Frame* frame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);

    vpsNal->setPresentationTime(timestamp);
    spsNal->setPresentationTime(timestamp);
    ppsNal->setPresentationTime(timestamp);
    prefixSeiNal->setPresentationTime(timestamp);
    suffixSeiNal->setPresentationTime(timestamp);

    frame = segmenter->manageFrame(vpsNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(spsNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(ppsNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(prefixSeiNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(suffixSeiNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());
}

void DashVideoSegmenterHEVCTest::manageIDRNals()
{
    Frame* frame;
    VideoFrame* vFrame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);
    int nalStartCodeLength = 4;
    int avccHeaderLength = 4;

    audNal->setPresentationTime(timestamp);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    idr1Nal->setPresentationTime(timestamp);
    idr1Nal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(idr1Nal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());
    
    idr2Nal->setPresentationTime(timestamp);
    idr2Nal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(idr2Nal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isPreviousFrameIntra());

    craNal->setPresentationTime(timestamp);
    craNal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(craNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isPreviousFrameIntra());

    audNal->setPresentationTime(timestamp + frameTime);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(segmenter->isPreviousFrameIntra());

    vFrame = dynamic_cast<VideoFrame*>(frame);

    CPPUNIT_ASSERT(vFrame);
    CPPUNIT_ASSERT(vFrame->getWidth() == WIDTH);
    CPPUNIT_ASSERT(vFrame->getHeight() == HEIGHT);
    CPPUNIT_ASSERT(vFrame->getPresentationTime() == timestamp);

    size_t totalLength = idr1Nal->getLength() + idr2Nal->getLength() + craNal->getLength() - nalStartCodeLength*3 + avccHeaderLength*3;

    CPPUNIT_ASSERT(vFrame->getLength() == totalLength);
}

void DashVideoSegmenterHEVCTest::manageNonIDRNals()
{
    Frame* frame;
    VideoFrame* vFrame;
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);
    int nalStartCodeLength = 4;
    int avccHeaderLength = 4;

    audNal->setPresentationTime(timestamp);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    nonTsaNal->setPresentationTime(timestamp);
    nonTsaNal->setSize(WIDTH, HEIGHT);
    frame = segmenter->manageFrame(nonTsaNal);
    CPPUNIT_ASSERT(!frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    audNal->setPresentationTime(timestamp + frameTime);
    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isPreviousFrameIntra());

    vFrame = dynamic_cast<VideoFrame*>(frame);

    CPPUNIT_ASSERT(vFrame);
    CPPUNIT_ASSERT(vFrame->getWidth() == WIDTH);
    CPPUNIT_ASSERT(vFrame->getHeight() == HEIGHT);
    CPPUNIT_ASSERT(vFrame->getPresentationTime() == timestamp);

    CPPUNIT_ASSERT(vFrame->getLength() == nonTsaNal->getLength() - nalStartCodeLength + avccHeaderLength);
}

void DashVideoSegmenterHEVCTest::generateInitSegment()
{
    char* initModel = new char[MAX_DAT];
    size_t initModelLength;
    DashSegment* initSegment = new DashSegment();
    std::chrono::microseconds ts(1000);
    Frame* frame;

    vpsNal->setPresentationTime(ts);
    spsNal->setPresentationTime(ts);
    ppsNal->setPresentationTime(ts);
    idr1Nal->setPresentationTime(ts);
    idr1Nal->setSize(WIDTH,HEIGHT);
    idr2Nal->setPresentationTime(ts);
    idr2Nal->setSize(WIDTH,HEIGHT);
    audNal->setPresentationTime(ts);

    initModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/initModel.m4v", initModel);

    frame = segmenter->manageFrame(vpsNal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(spsNal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(ppsNal);
    CPPUNIT_ASSERT(!frame);

    CPPUNIT_ASSERT(!segmenter->generateInitSegment(initSegment));

    frame = segmenter->manageFrame(idr1Nal);
    CPPUNIT_ASSERT(!frame);
    
    frame = segmenter->manageFrame(idr2Nal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(segmenter->generateInitSegment(initSegment));

    CPPUNIT_ASSERT(initModelLength == initSegment->getDataLength());
    CPPUNIT_ASSERT(memcmp(initModel, initSegment->getDataBuffer(), initSegment->getDataLength()) == 0);
}

void DashVideoSegmenterHEVCTest::appendFrameToDashSegment()
{
    Frame* frame = NULL;
    std::chrono::microseconds ts(1000);

    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(frame));

    idr1Nal->setPresentationTime(ts);
    idr1Nal->setSize(WIDTH, HEIGHT);
    audNal->setPresentationTime(ts);

    frame = segmenter->manageFrame(idr1Nal);
    CPPUNIT_ASSERT(!frame);

    frame = segmenter->manageFrame(audNal);
    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(segmenter->appendFrameToDashSegment(frame));
}

void DashVideoSegmenterHEVCTest::generateSegment()
{
    char* segmentModel = new char[MAX_DAT];
    size_t segmentModelLength;
    DashSegment* segment = new DashSegment();

    std::ofstream segmentS;

    Frame* frame = NULL;
    std::chrono::microseconds ts(1000);
    size_t nalCounter = 0;
    size_t dataLength = 0;
    std::string strName;

    segmentModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterHEVCTest/segmentModel.m4v", segmentModel);

    dummyNal->setSize(WIDTH, HEIGHT);
    CPPUNIT_ASSERT(!segmenter->generateSegment(segment, frame));

    while (true) {
        strName = "testsData/modules/dasher/dashVideoSegmenterHEVCTest/nalModels/nal_" + std::to_string(nalCounter);
        dataLength = readFile(strName.c_str(), (char*)dummyNal->getDataBuf());
        dummyNal->setLength(dataLength);
        dummyNal->setPresentationTime(ts);
        frame = segmenter->manageFrame(dummyNal);
        nalCounter++;

        if (!frame) {
            continue;
        }

        ts += frameTime;

        if (segmenter->generateSegment(segment, frame)) {
            break;
        }

        if(!segmenter->appendFrameToDashSegment(frame)) {
            CPPUNIT_FAIL("Segmenter appendFrameToDashSegment failed when testing general workflow\n");
        }
    }
    
    CPPUNIT_ASSERT(segmentModelLength == segment->getDataLength());
}

CPPUNIT_TEST_SUITE_REGISTRATION(DashVideoSegmenterAVCTest);
CPPUNIT_TEST_SUITE_REGISTRATION(DashVideoSegmenterHEVCTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("DashVideoSegmenterTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
