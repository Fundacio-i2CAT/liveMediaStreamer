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

#define SEG_DURATION 2
#define BASE_NAME "testsData/modules/dasher/dashAudioSegmenterTest/test"
#define CHANNELS 2
#define SAMPLE_RATE 48000
#define AAC_FRAME_SAMPLES 1024

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


class DashAudioSegmenterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DashAudioSegmenterTest);
    CPPUNIT_TEST(manageFrame);
    CPPUNIT_TEST(generateInitSegment);
    CPPUNIT_TEST(appendFrameToDashSegment);
    CPPUNIT_TEST(generateSegment);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void manageFrame();
    void generateInitSegment();
    void appendFrameToDashSegment();
    void generateSegment();

    bool newFrame;
    DashAudioSegmenter* segmenter;
    AudioFrame* modelFrame;
    VideoFrame *vFrame;
    
    std::chrono::microseconds timestamp = std::chrono::microseconds(0);
};

void DashAudioSegmenterTest::setUp()
{
    size_t dataLength;

    segmenter = new DashAudioSegmenter(std::chrono::seconds(SEG_DURATION), timestamp);
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
    Frame* frame;

    CPPUNIT_ASSERT(!segmenter->manageFrame(vFrame));

    frame = segmenter->manageFrame(modelFrame);
    CPPUNIT_ASSERT(frame);
}

void DashAudioSegmenterTest::generateInitSegment()
{
    char* initModel = new char[MAX_DAT];
    size_t initModelLength;
    DashSegment* initSegment = new DashSegment();
    std::chrono::microseconds ts = std::chrono::microseconds(1000);
    Frame* frame;

    initModelLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/initModel.m4a", initModel);
    modelFrame->setSamples(AAC_FRAME_SAMPLES);
    modelFrame->setPresentationTime(ts);

    frame = segmenter->manageFrame(modelFrame);

    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(segmenter->generateInitSegment(initSegment));
    CPPUNIT_ASSERT(!segmenter->generateInitSegment(initSegment));
    CPPUNIT_ASSERT(initModelLength == initSegment->getDataLength());
    CPPUNIT_ASSERT(memcmp(initModel, initSegment->getDataBuffer(), initSegment->getDataLength()) == 0);
}

void DashAudioSegmenterTest::appendFrameToDashSegment()
{
    Frame* frame;
    DashSegment* segment = new DashSegment();
    std::chrono::microseconds ts = std::chrono::microseconds(1000);

    modelFrame->setSamples(AAC_FRAME_SAMPLES);
    modelFrame->setPresentationTime(ts);

    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(segment, NULL));
    CPPUNIT_ASSERT(!segmenter->appendFrameToDashSegment(segment, modelFrame));

    frame = segmenter->manageFrame(modelFrame);

    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(segmenter->appendFrameToDashSegment(segment, frame));
}

void DashAudioSegmenterTest::generateSegment()
{
    std::string segName;
    size_t segmentModelLength;
    char* segmentModel = new char[MAX_DAT];
    DashSegment* segment = new DashSegment();

    Frame* frame;
    size_t samples = 0;
    std::chrono::microseconds originTs(1000);

    segmentModelLength = readFile("testsData/modules/dasher/dashAudioSegmenterTest/segmentModel.m4a", segmentModel);

    modelFrame->setSamples(AAC_FRAME_SAMPLES);
    CPPUNIT_ASSERT(!segmenter->generateSegment(segment, NULL));
    CPPUNIT_ASSERT(!segmenter->generateSegment(segment, modelFrame));

    while (true) {
        modelFrame->setPresentationTime(std::chrono::microseconds(samples*std::micro::den/SAMPLE_RATE) + originTs);
        frame = segmenter->manageFrame(modelFrame);

        CPPUNIT_ASSERT(frame);
        samples += AAC_FRAME_SAMPLES;

        if (segmenter->generateSegment(segment, frame)) {
            break;
        }

        CPPUNIT_ASSERT(segmenter->appendFrameToDashSegment(segment, frame));
    }

    CPPUNIT_ASSERT(segmentModelLength == segment->getDataLength());
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

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
