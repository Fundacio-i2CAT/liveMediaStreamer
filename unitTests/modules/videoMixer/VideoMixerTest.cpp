/*
 *  VideoMixerTest.cpp - VideoMixer class test
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

#include "modules/videoMixer/VideoMixer.hh"

class VideoMixerMock : public VideoMixer {

public:
    VideoMixerMock(int channels, int width, int height, std::chrono::microseconds fTime) :
    VideoMixer(MASTER, channels, width, height, fTime) {}; 
    using VideoMixer::setReader;
};

class VideoMixerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VideoMixerTest);
    CPPUNIT_TEST(constructorTest);
    CPPUNIT_TEST(channelConfigTest);
    CPPUNIT_TEST_SUITE_END();

protected:
    void constructorTest();
    void channelConfigTest();

    int width = 1920;
    int height = 1080;
    int channels = 8;
};

void VideoMixerTest::constructorTest()
{
    VideoMixer* mixer;

    int negWidth = -1280;
    int negHeight = -720;
    int zeroWidth = 0;
    int zeroHeight = 0;
    int tooLargeWidth = 3840;
    int tooLargeHeight = 2160;
    std::chrono::microseconds negTime(-200);

    mixer = VideoMixer::createNew(MASTER, channels, negWidth, height);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, zeroWidth, height);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, tooLargeWidth, height);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, width, negHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, width, zeroHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, width, tooLargeHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, negWidth, negHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, negWidth, zeroHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, negWidth, tooLargeHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, zeroWidth, negHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, zeroWidth, zeroHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, zeroWidth, tooLargeHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, tooLargeWidth, negHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, tooLargeWidth, zeroHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, tooLargeWidth, tooLargeHeight);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, width, height, negTime);
    CPPUNIT_ASSERT(!mixer);
    mixer = VideoMixer::createNew(MASTER, channels, width, height);
    CPPUNIT_ASSERT(mixer);

    delete mixer;
}

void VideoMixerTest::channelConfigTest()
{
    int id = 1;
    std::chrono::microseconds fTime(0);
    VideoMixerMock* mixer;
    std::shared_ptr<Reader> r;

    mixer = new VideoMixerMock(channels, width, height, fTime);

    CPPUNIT_ASSERT(!mixer->configChannel(id, 1.0, 1.0, 0.0, 0.0, 1, true, 1.0));
    r = mixer->setReader(id, NULL);
    CPPUNIT_ASSERT(r);
    r = mixer->setReader(id, NULL);
    CPPUNIT_ASSERT(!r);

    CPPUNIT_ASSERT(mixer->configChannel(id, 1, 1, 0, 0, 1, true, 1));
    
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, -1, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, 0, -1, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, -1, 0, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 0, 0, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, -1, 1, 0, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 0, 1, 0, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, 0, 0, 1, true, -1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, 0, 0, 1, true, 1.01));
    
    CPPUNIT_ASSERT(!mixer->configChannel(id, 0.6, 1, 0.7, 0, 1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 0.8, 0, 0.4, 1, true, 1));

    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, 0, 0, -1, true, 1));
    CPPUNIT_ASSERT(!mixer->configChannel(id, 1, 1, 0, 0, mixer->getMaxChannels()+1, true, 1));

    delete mixer;
}

CPPUNIT_TEST_SUITE_REGISTRATION(VideoMixerTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("VideoMixerTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}