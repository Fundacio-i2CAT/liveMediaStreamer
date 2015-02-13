/*
 *  DashAudioSegmenterTest.cpp - DashVideoSegmenter class test
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

#include "modules/dasher/DashAudioSegmenter.hh"

#define SEG_DURATION 2000000
#define BASE_NAME "testsData/modules/dasher/dashAudioSegmenterTest/test"
#define CHANNELS 2
#define SAMPLE_RATE 48000
#define AAC_FRAME_SAMPLES 1024

size_t readFile(char const* fileName, char* dstBuffer) 
{
    size_t inputDataSize;
    std::ifstream inputDataFile(fileName, std::ios::in|std::ios::binary|std::ios::ate);
    
    if (!inputDataFile.is_open()) {
        CPPUNIT_FAIL("Test data upload failed. Check test data file paths\n");
        return 0;
    }

    inputDataSize = inputDataFile.tellg();
    inputDataFile.seekg (0, std::ios::beg);
    inputDataFile.read(dstBuffer, inputDataSize);
    return inputDataSize;
}


class DashAudioSegmenterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DashAudioSegmenterTest);
    CPPUNIT_TEST(manageFrame);
    CPPUNIT_TEST(updateConfig);
    CPPUNIT_TEST(generateSegmentAndInitSegment);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageFrame();
    void updateConfig();
    void generateSegmentAndInitSegment();

    bool newFrame;
    DashAudioSegmenter* segmenter;
    AudioFrame* modelFrame;
    VideoFrame *vFrame;
};

void DashAudioSegmenterTest::setUp()
{
    size_t dataLength;

    segmenter = new DashAudioSegmenter(SEG_DURATION, BASE_NAME);
    modelFrame = InterleavedAudioFrame::createNew(CHANNELS, SAMPLE_RATE, AudioFrame::getMaxSamples(SAMPLE_RATE), AAC, S16);

    dataLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/modelFrame.aac", (char*)modelFrame->getDataBuf());
    modelFrame->setLength(dataLength);
}

void DashAudioSegmenterTest::tearDown()
{
    delete modelFrame;
}

void DashAudioSegmenterTest::manageFrame()
{
    CPPUNIT_ASSERT(!segmenter->manageFrame(vFrame, newFrame));
    CPPUNIT_ASSERT(segmenter->manageFrame(modelFrame, newFrame));
    CPPUNIT_ASSERT(newFrame);
}

void DashAudioSegmenterTest::updateConfig()
{
    std::chrono::microseconds ts(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
    std::chrono::microseconds ts0(0);

    modelFrame->setPresentationTime(ts0);
    segmenter->manageFrame(modelFrame, newFrame);
    CPPUNIT_ASSERT(!segmenter->updateConfig());

    modelFrame->setPresentationTime(ts);
    modelFrame->setSamples(AAC_FRAME_SAMPLES);
    segmenter->manageFrame(modelFrame, newFrame);
    CPPUNIT_ASSERT(segmenter->updateConfig());
    CPPUNIT_ASSERT(segmenter->getFrameDuration() == AAC_FRAME_SAMPLES);
    CPPUNIT_ASSERT(segmenter->getTimeBase() == SAMPLE_RATE);
    CPPUNIT_ASSERT(segmenter->getTsOffset() == (size_t)ts.count());
    CPPUNIT_ASSERT(segmenter->getCustomSegmentDuration() == (SEG_DURATION*segmenter->getTimeBase())/MICROSECONDS_TIME_BASE);
}

void DashAudioSegmenterTest::generateSegmentAndInitSegment()
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
    std::chrono::microseconds frameTime(21333);
    std::chrono::microseconds ts(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
    std::string strName;

    initModelLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/initModel.m4a", initModel);
    segmentModelLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/segmentModel.m4a", segmentModel);

    modelFrame->setSamples(AAC_FRAME_SAMPLES);
    
    while(!haveInit || !haveSegment) {
        modelFrame->setPresentationTime(ts);
        segmenter->manageFrame(modelFrame, newFrame);

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

    initLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/test_init.m4a", init);
    segmentLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/test_0.m4a", segment);

    CPPUNIT_ASSERT(initModelLength == initLength);
    CPPUNIT_ASSERT(segmentModelLength == segmentLength);
    
    CPPUNIT_ASSERT(memcmp(initModel, init, initModelLength) == 0);
    CPPUNIT_ASSERT(memcmp(segmentModel, segment, segmentModelLength) == 0);

    delete initModel;
    delete init;
    delete segmentModel;
    delete segment;
}

CPPUNIT_TEST_SUITE_REGISTRATION(DashAudioSegmenterTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("DashAudioSegmenterTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
} 