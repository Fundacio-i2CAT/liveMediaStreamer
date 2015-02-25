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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
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

#define SEG_DURATION 2
#define BASE_NAME "testsData/modules/dasher/dashVideoSegmenterTest/test"
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

class DashVideoSegmenterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DashVideoSegmenterTest);
    CPPUNIT_TEST(manageInvalidFrames);
    CPPUNIT_TEST(manageSetInternalValues);
    CPPUNIT_TEST(manageNonVCLNals);
    CPPUNIT_TEST(manageIDRNals);
    CPPUNIT_TEST(manageNonIDRNals);
    CPPUNIT_TEST(updateConfig);
    CPPUNIT_TEST(generateInitSegment);
    CPPUNIT_TEST(appendFrameToDashSegment);
    CPPUNIT_TEST(generateSegment);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageInvalidFrames();
    void manageSetInternalValues();
    void manageNonVCLNals();
    void manageIDRNals();
    void manageNonIDRNals();
    void updateConfig();
    void generateSegmentAndInitSegment();
    void generateInitSegment();
    void appendFrameToDashSegment();
    void generateSegment();

    DashVideoSegmenter* segmenter;
    AudioFrame* aFrame;
    VideoFrame *spsNal;
    VideoFrame *ppsNal;
    VideoFrame *seiNal;
    VideoFrame *audNal;
    VideoFrame *idrNal;
    VideoFrame *nonIdrNal;
    VideoFrame *dummyNal;
    std::chrono::microseconds frameTime;
    std::chrono::nanoseconds nanoFrameTime;
};

void DashVideoSegmenterTest::setUp()
{
    size_t dataLength;

    frameTime = std::chrono::microseconds(40000);
    nanoFrameTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameTime);


    segmenter = new DashVideoSegmenter(std::chrono::seconds(SEG_DURATION));
    dummyNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);

    spsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    ppsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    seiNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    audNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    idrNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdrNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_sps", (char*)spsNal->getDataBuf());
    spsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_pps", (char*)ppsNal->getDataBuf());
    ppsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_sei", (char*)seiNal->getDataBuf());
    seiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_aud", (char*)audNal->getDataBuf());
    audNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_idr_0", (char*)idrNal->getDataBuf());
    idrNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_nonIdr0", (char*)nonIdrNal->getDataBuf());
    nonIdrNal->setLength(dataLength);
}

void DashVideoSegmenterTest::tearDown()
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

void DashVideoSegmenterTest::manageInvalidFrames()
{
    bool newFrame;

    CPPUNIT_ASSERT(!segmenter->manageFrame(aFrame, newFrame));
    CPPUNIT_ASSERT(!segmenter->manageFrame(dummyNal, newFrame));
}

void DashVideoSegmenterTest::manageSetInternalValues()
{
    size_t tsValue = 1000;
    bool newFrame;

    std::chrono::microseconds timestamp(tsValue);
    idrNal->setPresentationTime(std::chrono::system_clock::time_point(timestamp));
    idrNal->setSize(WIDTH, HEIGHT);

    CPPUNIT_ASSERT(segmenter->manageFrame(idrNal, newFrame));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == std::chrono::system_clock::time_point(timestamp));
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);
}

void DashVideoSegmenterTest::manageNonVCLNals()
{
    bool newFrame;

    CPPUNIT_ASSERT(segmenter->manageFrame(spsNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getSPSsize() > 0);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(ppsNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getPPSsize() > 0);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(seiNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());
}

void DashVideoSegmenterTest::manageIDRNals()
{
    bool newFrame;

    CPPUNIT_ASSERT(segmenter->manageFrame(idrNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal, newFrame));
    CPPUNIT_ASSERT(newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());
}

void DashVideoSegmenterTest::manageNonIDRNals()
{
    bool newFrame;

    CPPUNIT_ASSERT(segmenter->manageFrame(nonIdrNal, newFrame));
    CPPUNIT_ASSERT(!newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal, newFrame));
    CPPUNIT_ASSERT(newFrame);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());
}

void DashVideoSegmenterTest::updateConfig()
{
    bool newFrame;

    CPPUNIT_ASSERT(!segmenter->updateConfig());

    std::chrono::system_clock::time_point ts = std::chrono::system_clock::now();
    idrNal->setPresentationTime(std::chrono::system_clock::time_point(ts));
    idrNal->setDuration(nanoFrameTime);
    idrNal->setSize(WIDTH, HEIGHT);
    audNal->setPresentationTime(std::chrono::system_clock::time_point(ts));
    audNal->setDuration(nanoFrameTime);
    audNal->setSize(WIDTH, HEIGHT);

    segmenter->manageFrame(idrNal, newFrame);
    segmenter->manageFrame(audNal, newFrame);

    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getFramerate() == (size_t)std::nano::den/nanoFrameTime.count());
}

void DashVideoSegmenterTest::generateInitSegment()
{
    char* initModel = new char[MAX_DAT];
    size_t initModelLength;
    DashSegment* initSegment = new DashSegment();
    std::chrono::system_clock::time_point ts = std::chrono::system_clock::now();
    bool newFrame;

    spsNal->setSize(WIDTH, HEIGHT);
    ppsNal->setSize(WIDTH, HEIGHT);
    spsNal->setPresentationTime(ts);
    ppsNal->setPresentationTime(ts);
    spsNal->setDuration(nanoFrameTime);
    ppsNal->setDuration(nanoFrameTime);

    initModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/initModel.m4v", initModel);

    segmenter->manageFrame(spsNal, newFrame);
    segmenter->manageFrame(ppsNal, newFrame);

    if (!segmenter->updateConfig()) {
        CPPUNIT_FAIL("Segmenter updateConfig failed when testing general workflow\n");
    }

    CPPUNIT_ASSERT(segmenter->generateInitSegment(initSegment));
    CPPUNIT_ASSERT(!segmenter->generateInitSegment(initSegment));
    CPPUNIT_ASSERT(initModelLength == initSegment->getDataLength());
    CPPUNIT_ASSERT(memcmp(initModel, initSegment->getDataBuffer(), initSegment->getDataLength()) == 0);

}

void DashVideoSegmenterTest::appendFrameToDashSegment()
{
    bool newFrame;
    DashSegment* segment = new DashSegment();
    std::chrono::system_clock::time_point ts = std::chrono::system_clock::now();

    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(segment));

    idrNal->setPresentationTime(ts);
    idrNal->setDuration(nanoFrameTime);
    idrNal->setSize(WIDTH, HEIGHT);
    nonIdrNal->setPresentationTime(ts);
    nonIdrNal->setDuration(nanoFrameTime);
    nonIdrNal->setSize(WIDTH, HEIGHT);

    segmenter->manageFrame(nonIdrNal, newFrame);

    if (!segmenter->updateConfig()) {
        CPPUNIT_FAIL("Segmenter updateConfig failed when testing general workflow\n");
    }

    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(segment));
    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(segment));

    segmenter->manageFrame(idrNal, newFrame);

    CPPUNIT_ASSERT(segmenter->appendFrameToDashSegment(segment));
}

void DashVideoSegmenterTest::generateSegment()
{
    char* segmentModel = new char[MAX_DAT];
    size_t segmentModelLength;
    DashSegment* segment = new DashSegment();

    bool newFrame;
    bool haveSegment = false;
    std::chrono::system_clock::time_point ts(std::chrono::microseconds(1000));
    size_t nalCounter = 0;
    size_t dataLength = 0;
    std::string strName;

    segmentModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/segmentModel.m4v", segmentModel);

    dummyNal->setSize(WIDTH, HEIGHT);

    CPPUNIT_ASSERT(!segmenter->generateSegment(segment));

    while (!haveSegment) {
        strName = "testsData/modules/dasher/dashVideoSegmenterTest/nalModels/nal_" + std::to_string(nalCounter);
        dataLength = readFile(strName.c_str(), (char*)dummyNal->getDataBuf());
        dummyNal->setLength(dataLength);
        dummyNal->setPresentationTime(ts);
        dummyNal->setDuration(std::chrono::duration_cast<std::chrono::nanoseconds>(frameTime));
        segmenter->manageFrame(dummyNal, newFrame);
        nalCounter++;

        if (!newFrame) {
            continue;
        }

        if (!segmenter->updateConfig()) {
            CPPUNIT_FAIL("Segmenter updateConfig failed when testing general workflow\n");
        }
        ts += frameTime;

        if (segmenter->generateSegment(segment)) {
            haveSegment = true;
        }

        if(!segmenter->appendFrameToDashSegment(segment)) {
            CPPUNIT_FAIL("Segmenter appendFrameToDashSegment failed when testing general workflow\n");
        }
    }

    CPPUNIT_ASSERT(segmentModelLength == segment->getDataLength());
    CPPUNIT_ASSERT(memcmp(segmentModel, segment->getDataBuffer(), segment->getDataLength()) == 0);

}

CPPUNIT_TEST_SUITE_REGISTRATION(DashVideoSegmenterTest);

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
