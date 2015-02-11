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

#define SEG_DURATION 4000000
#define BASE_NAME "test"
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
    CPPUNIT_TEST(manageNonVCLNals);
    CPPUNIT_TEST(manageIDRNals);
    CPPUNIT_TEST(manageNonIDRNals);
    CPPUNIT_TEST(updateConfig);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageNonVCLNals();
    void manageIDRNals();
    void manageNonIDRNals();
    void updateConfig();
    void generateInitSegment();
    void generateSegment();

    DashVideoSegmenter* segmenter;
    AudioFrame* aFrame;
    VideoFrame *spsNal, *ppsNal, *seiNal, *audNal;
    VideoFrame *idr0Nal, *idr1Nal, *idr2Nal, *idr3Nal;
    VideoFrame *nonIdr0Nal, *nonIdr1Nal, *nonIdr2Nal, *nonIdr3Nal;
};

void DashVideoSegmenterTest::setUp()
{
    size_t dataLength;

    segmenter = new DashVideoSegmenter(SEG_DURATION, BASE_NAME);
    
    idr0Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    idr1Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    idr2Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    idr3Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    spsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    ppsNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    seiNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    audNal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdr0Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdr1Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdr2Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);
    nonIdr3Nal = InterleavedVideoFrame::createNew(H264, LENGTH_H264);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_idr_0", (char*)idr0Nal->getDataBuf());
    idr0Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_idr_1", (char*)idr1Nal->getDataBuf());
    idr1Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_idr_2", (char*)idr2Nal->getDataBuf());
    idr2Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_idr_3", (char*)idr3Nal->getDataBuf());
    idr3Nal->setLength(dataLength);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_sps", (char*)spsNal->getDataBuf());
    spsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_pps", (char*)ppsNal->getDataBuf());
    ppsNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_sei", (char*)seiNal->getDataBuf());
    seiNal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_aud", (char*)audNal->getDataBuf());
    audNal->setLength(dataLength);

    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_nonIdr0", (char*)nonIdr0Nal->getDataBuf());
    nonIdr0Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_nonIdr1", (char*)nonIdr1Nal->getDataBuf());
    nonIdr1Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_nonIdr2", (char*)nonIdr2Nal->getDataBuf());
    nonIdr2Nal->setLength(dataLength);
    dataLength = readFile("testsData/modules/dasher/dashVideoSegmenterTest/nal_nonIdr3", (char*)nonIdr3Nal->getDataBuf());
    nonIdr3Nal->setLength(dataLength);
    
}

void DashVideoSegmenterTest::tearDown()
{
    delete segmenter;
    delete spsNal;
    delete ppsNal;
    delete seiNal;
    delete audNal;
    delete idr0Nal;
    delete idr1Nal;
    delete idr2Nal;
    delete idr3Nal;
    delete nonIdr0Nal;
    delete nonIdr1Nal;
    delete nonIdr2Nal;
    delete nonIdr3Nal;
}


void DashVideoSegmenterTest::manageNonVCLNals()
{
    CPPUNIT_ASSERT(!segmenter->manageFrame(aFrame));

    CPPUNIT_ASSERT(!segmenter->manageFrame(spsNal));
    CPPUNIT_ASSERT(segmenter->getSPSsize() > 0);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(ppsNal));
    CPPUNIT_ASSERT(segmenter->getPPSsize() > 0);
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(seiNal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(audNal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() <= 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(!segmenter->isVCLFrame());
}

void DashVideoSegmenterTest::manageIDRNals()
{
    CPPUNIT_ASSERT(!segmenter->manageFrame(idr0Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr1Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr2Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr3Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());
}

void DashVideoSegmenterTest::manageNonIDRNals()
{
    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr0Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr1Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr2Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr3Nal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal));
    CPPUNIT_ASSERT(segmenter->getFrameDataSize() > 0);
    CPPUNIT_ASSERT(!segmenter->isIntraFrame());
    CPPUNIT_ASSERT(segmenter->isVCLFrame());
}


void DashVideoSegmenterTest::updateConfig()
{
    std::chrono::microseconds frameTime(40000);
    CPPUNIT_ASSERT(!segmenter->updateConfig());

    std::chrono::microseconds ts(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
    idr0Nal->setPresentationTime(ts);
    idr0Nal->setSize(WIDTH, HEIGHT);

    idr1Nal->setPresentationTime(ts);
    idr1Nal->setSize(WIDTH, HEIGHT);

    idr2Nal->setPresentationTime(ts);
    idr2Nal->setSize(WIDTH, HEIGHT);

    idr3Nal->setPresentationTime(ts);
    idr3Nal->setSize(WIDTH, HEIGHT);

    audNal->setPresentationTime(ts);
    audNal->setSize(WIDTH, HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr0Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr1Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr2Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(idr3Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getLastTs() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getTsOffset() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getFramerate() == VIDEO_DEFAULT_FRAMERATE);
    CPPUNIT_ASSERT(segmenter->getFrameDuration() == segmenter->getTimeBase()/VIDEO_DEFAULT_FRAMERATE);

    std::chrono::microseconds ts2(ts + frameTime);
    nonIdr0Nal->setPresentationTime(ts2);
    nonIdr0Nal->setSize(WIDTH, HEIGHT);

    nonIdr1Nal->setPresentationTime(ts2);
    nonIdr1Nal->setSize(WIDTH, HEIGHT);

    nonIdr2Nal->setPresentationTime(ts2);
    nonIdr2Nal->setSize(WIDTH, HEIGHT);

    nonIdr3Nal->setPresentationTime(ts2);
    nonIdr3Nal->setSize(WIDTH, HEIGHT);

    audNal->setPresentationTime(ts2);
    audNal->setSize(WIDTH, HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr0Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr1Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr2Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(!segmenter->manageFrame(nonIdr3Nal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(segmenter->manageFrame(audNal));
    CPPUNIT_ASSERT(segmenter->getCurrentTimestamp() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getWidth() == WIDTH);
    CPPUNIT_ASSERT(segmenter->getHeight() == HEIGHT);

    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getLastTs() == (size_t)ts2.count());
    CPPUNIT_ASSERT(segmenter->getTsOffset() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getFrameDuration() == (size_t)(ts2.count() - ts.count()));
    CPPUNIT_ASSERT(segmenter->getFramerate() == segmenter->getTimeBase()/segmenter->getFrameDuration());
}

void DashVideoSegmenterTest::generateInitSegment()
{

}

void DashVideoSegmenterTest::generateSegment()
{

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