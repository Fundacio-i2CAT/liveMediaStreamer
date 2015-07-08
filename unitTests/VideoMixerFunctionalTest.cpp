/*
 *  VideoMixerFunctionalTest.cpp - VideoMixingFunctionalTest class test
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
#include <chrono>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "FilterFunctionalMockup.hh"
#include "modules/videoMixer/VideoMixer.hh"

class VideoMixerFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VideoMixerFunctionalTest);
    CPPUNIT_TEST(mixingTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void mixingTest();

    int mixWidth = 1920;
    int mixHeight = 1080;
    int channels = 8;

    ManyToOneVideoScenarioMockup *mixScenario;
    VideoMixer* mixer;
    AVFramesReader* reader;
};

void VideoMixerFunctionalTest::setUp()
{
    mixer = VideoMixer::createNew(MASTER, channels, mixWidth, mixHeight);
    mixScenario = new ManyToOneVideoScenarioMockup(mixer);
    reader = new AVFramesReader();

    CPPUNIT_ASSERT(mixScenario->addHeadFilter(1, RAW, RGB24)); 
    CPPUNIT_ASSERT(mixScenario->addHeadFilter(2, RAW, RGB24)); 
    CPPUNIT_ASSERT(mixScenario->addHeadFilter(3, RAW, RGB24));
    CPPUNIT_ASSERT(mixScenario->connectFilters());
    CPPUNIT_ASSERT(mixer->configChannel(1, 0.5, 0.5, 0, 0, 1, true, 1));
    CPPUNIT_ASSERT(mixer->configChannel(2, 0.5, 0.5, 0.3, 0.1, 1, true, 1));
    CPPUNIT_ASSERT(mixer->configChannel(3, 0.5, 0.5, 0.4, 0.4, 1, true, 1));
}

void VideoMixerFunctionalTest::tearDown()
{
    delete mixScenario;
    delete mixer;
    delete reader;
}

void VideoMixerFunctionalTest::mixingTest()
{
    InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame *mixedFrame = NULL;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoMixerFunctionalTestInputImage.rgb", RAW, RGB24, mixWidth/2, mixHeight/2));

    frame = reader->getFrame();
    CPPUNIT_ASSERT(frame);
    mixScenario->processFrame(frame);
    mixedFrame = mixScenario->extractFrame();
    CPPUNIT_ASSERT(mixedFrame);

    CPPUNIT_ASSERT(mixedFrame->getWidth() == mixWidth && mixedFrame->getHeight() == mixHeight);
    reader->close();

    CPPUNIT_ASSERT(reader->openFile("testsData/videoMixerFunctionalTestModel.rgb", RAW, RGB24, mixWidth, mixHeight));
    frame = reader->getFrame();
    CPPUNIT_ASSERT(frame);

    CPPUNIT_ASSERT(frame->getLength() == mixedFrame->getLength());
    CPPUNIT_ASSERT(frame->getWidth() == mixedFrame->getWidth());
    CPPUNIT_ASSERT(frame->getHeight() == mixedFrame->getHeight());

    CPPUNIT_ASSERT(memcmp(frame->getDataBuf(), mixedFrame->getDataBuf(), frame->getLength()) == 0);
}

CPPUNIT_TEST_SUITE_REGISTRATION(VideoMixerFunctionalTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("VideoMixerFunctionalTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}