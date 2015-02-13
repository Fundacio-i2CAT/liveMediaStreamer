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

#define SEG_DURATION 2000000
#define BASE_NAME "testsData/modules/dasher/dashVideoSegmenterTest/test"
#define TIMEBASE 1000000
#define SAMPLE_DURATION 40000
#define WIDTH 1280
#define HEIGHT 534
#define FRAMERATE 25

size_t readFile(char const* fileName, char* dstBuffer) 
{
    size_t inputDataSize;
    std::ifstream inputDataFile(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    
    if (!inputDataFile.is_open()) {
        CPPUNIT_FAIL("Test data upload failed. Check test data file paths\n");
        return 0;
    }

    inputDataSize = inputDataFile.tellg();
    // std::cout << fileName << "    " << inputDataSize << std::endl;
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
    CPPUNIT_TEST(generateSegmentAndInitSegment);
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

    bool newFrame;
    DashVideoSegmenter* segmenter;
    AudioFrame* aFrame;
    VideoFrame *spsNal;
    VideoFrame *ppsNal;
    VideoFrame *seiNal;
    VideoFrame *audNal;
    VideoFrame *idrNal;
    VideoFrame *nonIdrNal;
    VideoFrame *dummyNal;
};

void DashVideoSegmenterTest::setUp()
{
    size_t dataLength;

    segmenter = new DashVideoSegmenter(SEG_DURATION, BASE_NAME);
    dummyNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    
    spsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    ppsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    seiNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    audNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    idrNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdrNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_sps", (char*)spsNal->getDataBuf());
    spsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_pps", (char*)ppsNal->getDataBuf());
    ppsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_sei", (char*)seiNal->getDataBuf());
    seiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_aud", (char*)audNal->getDataBuf());
    audNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_idr_0", (char*)idrNal->getDataBuf());
    idrNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_nonIdr0", (char*)nonIdrNal->getDataBuf());
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
    CPPUNIT_ASSERT(!segmenter->manageFrame(aFrame, newFrame));
    CPPUNIT_ASSERT(!segmenter->manageFrame(dummyNal, newFrame));
}

void DashVideoSegmenterTest::manageSetInternalValues()
{
    size_t tsValue = 1000;
    std::chrono::microseconds timestamp(tsValue);
    idrNal->setPresentationTime(timestamp);
    idrNal->setSize(WIDTH, HEIGHT);

    CPPUNIT_ASSERT(segmenter->manageFrame(idrNal, newFrame));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)timestamp.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);
}


void DashVideoSegmenterTest::manageNonVCLNals()
{
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
    std::chrono::microseconds frameTime(40000);
    CPPUNIT_ASSERT(!segmenter->updateConfig());

    std::chrono::microseconds ts(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
    std::chrono::microseconds ts0(0);
    idrNal->setPresentationTime(ts0);
    idrNal->setSize(WIDTH, HEIGHT);
    audNal->setPresentationTime(ts0);
    audNal->setSize(WIDTH, HEIGHT);

    segmenter->manageFrame(idrNal, newFrame);
    segmenter->manageFrame(audNal, newFrame);

    CPPUNIT_ASSERT(!segmenter->updateConfig());

    idrNal->setPresentationTime(ts);
    audNal->setPresentationTime(ts);
    
    segmenter->manageFrame(idrNal, newFrame);
    segmenter->manageFrame(audNal, newFrame);

    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getLastTs() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getTsOffset() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getFramerate() == VIDEO_DEFAULT_FRAMERATE);
    CPPUNIT_ASSERT(segmenter->getFrameDuration() == segmenter->getTimeBase()/VIDEO_DEFAULT_FRAMERATE);

    std::chrono::microseconds ts2(ts + frameTime);
    nonIdrNal->setPresentationTime(ts2);
    nonIdrNal->setSize(WIDTH, HEIGHT);
    audNal->setPresentationTime(ts2);
    audNal->setSize(WIDTH, HEIGHT);

    segmenter->manageFrame(idrNal, newFrame);
    segmenter->manageFrame(audNal,newFrame);

    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getLastTs() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getTsOffset() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getFrameDuration() == (size_t)(ts2.count() - ts.count()));
    CPPUNIT_ASSERT(segmenter->getFramerate() == segmenter->getTimeBase()/segmenter->getFrameDuration());    
}

void DashVideoSegmenterTest::generateSegmentAndInitSegment()
{
    char* initModel = new char[MAX_DAT];
    char* init = new char[MAX_DAT];
    size_t initModelLength;
    size_t initLength;
    char* segmentModel = new char[MAX_DAT];
    char* segment = new char[MAX_DAT];
    size_t segmentModelLength;
    size_t segmentLength;

    bool newFrame;
    bool haveInit = false;
    bool haveSegment = false;
    std::chrono::microseconds frameTime(40000);
    std::chrono::microseconds ts(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
    size_t nalCounter = 0;
    size_t dataLength = 0;
    std::string strName;

    initModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/initModel.m4v", initModel);
    segmentModelLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/segmentModel.m4v", segmentModel);

    dummyNal->setSize(WIDTH, HEIGHT);
    
    while(!haveInit || !haveSegment) {
        strName = "testsData/modules/dasher/dashVideoSegmenterTest/nal_" + std::to_string(nalCounter);
        dataLength = readFile(strName.c_str(), (char*)dummyNal->getDataBuf());
        dummyNal->setLength(dataLength);
        dummyNal->setPresentationTime(ts);
        segmenter->manageFrame(dummyNal, newFrame);
        nalCounter++;

        if(!newFrame) {
            continue;
        }

        if(!segmenter->updateConfig()) {
            CPPUNIT_FAIL("Segmenter updateConfig failed when testing general workflow\n");
        }
        ts += frameTime;

        if (segmenter->generateInitSegment()) {
            haveInit = true;
        }

        if (segmenter->generateSegment()) {
            haveSegment = true;
        }
    }

    initLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/test_init.m4v", init);
    segmentLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/test_0.m4v", segment);

    CPPUNIT_ASSERT(initModelLength == initLength);
    CPPUNIT_ASSERT(segmentModelLength == segmentLength);
    
    CPPUNIT_ASSERT(memcmp(initModel, init, initModelLength) == 0);
    CPPUNIT_ASSERT(memcmp(segmentModel, segment, segmentModelLength) == 0);
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

    return runner.result().wasSuccessful() ? 0 : 1;
} 