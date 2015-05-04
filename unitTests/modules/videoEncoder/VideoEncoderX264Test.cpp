/*
 *  VideoEncoderX264Test.cpp - VideoEncoderX264 class test
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

#include "modules/videoEncoder/VideoEncoderX264.hh"

class VideoEncoderX264Test : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VideoEncoderX264Test);
    CPPUNIT_TEST(reconfigureTest);
    CPPUNIT_TEST(fillPicturePlanesTest);
    CPPUNIT_TEST(encodeFrameTest);
    CPPUNIT_TEST(encodeHeadersFrameTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void fillPicturePlanesTest();
    void encodeFrameTest();
    void reconfigureTest();
    void encodeHeadersFrameTest();

    VideoEncoderX264* encoder;
};

void VideoEncoderX264Test::setUp()
{
    encoder = new VideoEncoderX264();
}

void VideoEncoderX264Test::tearDown()
{
    delete encoder;
}

void VideoEncoderX264Test::configure()
{
    std::string invalidFolder("nonExistance");
    CPPUNIT_ASSERT(!dasher->configure(invalidFolder, BASE_NAME, SEG_DURATION, MPD_LOCATION));
    CPPUNIT_ASSERT(!dasher->configure(DASH_FOLDER, "", SEG_DURATION, MPD_LOCATION));
    CPPUNIT_ASSERT(!dasher->configure(DASH_FOLDER, BASE_NAME, 0, MPD_LOCATION));
    CPPUNIT_ASSERT(!dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, ""));
    CPPUNIT_ASSERT(dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION));
}

void VideoEncoderX264Test::addSegmenter()
{
    CPPUNIT_ASSERT(!dasher->addSegmenter(h264ReaderId));

    if(!dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION)) {
        CPPUNIT_FAIL("Dasher creation failed");
    }

    CPPUNIT_ASSERT(!dasher->addSegmenter(nonExistanceReader));
    CPPUNIT_ASSERT(!dasher->addSegmenter(vp8ReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(mp3ReaderId));
    CPPUNIT_ASSERT(dasher->addSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(dasher->addSegmenter(aacReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(aacReaderId));
}

void VideoEncoderX264Test::removeSegmenter()
{
    if(!dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION)) {
        CPPUNIT_FAIL("Dasher configuration failed");
    }

    dasher->addSegmenter(h264ReaderId);
    dasher->addSegmenter(aacReaderId);
    CPPUNIT_ASSERT(dasher->removeSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(dasher->removeSegmenter(aacReaderId));
    CPPUNIT_ASSERT(!dasher->removeSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(!dasher->removeSegmenter(aacReaderId));
}

void VideoEncoderX264Test::setDashSegmenterBitrate() 
{
    dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION);
    dasher->addSegmenter(h264ReaderId);
    CPPUNIT_ASSERT(dasher->setDashSegmenterBitrate(h264ReaderId, 1000000));
    CPPUNIT_ASSERT(!dasher->setDashSegmenterBitrate(aacReaderId, 1000000));
}

CPPUNIT_TEST_SUITE_REGISTRATION(VideoEncoderX264Test);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("VideoEncoderX264Test.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
