/*
 *  DasherTest.cpp - Dasher filter class test
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

#include "modules/dasher/Dasher.hh"
#include "FilterMockup.hh"

#define SEG_DURATION 2 //sec
#define DASH_FOLDER "testsData/modules/dasher"
#define BASE_NAME "test"
#define MPD_LOCATION "http://localhost/testsData/modules/dasher/test.mpd"

class DasherTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DasherTest);
    CPPUNIT_TEST(configure);
    CPPUNIT_TEST(addSegmenter);
    CPPUNIT_TEST(removeSegmenter);
    CPPUNIT_TEST(setDashSegmenterBitrate);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void configure();
    void addSegmenter();
    void removeSegmenter();
    void setDashSegmenterBitrate();

    Dasher* dasher;
    VideoFilterMockup* h264Filter = NULL;
    VideoFilterMockup* vp8Filter = NULL;
    AudioFilterMockup* aacFilter = NULL;
    AudioFilterMockup* mp3Filter = NULL;
    int h264ReaderId = 100;
    int vp8ReaderId = 200;
    int aacReaderId = 300;
    int mp3ReaderId = 400;
    int nonExistanceReader = 500;
    std::string dashFolder = std::string(DASH_FOLDER);
    std::string baseName = std::string(BASE_NAME);
};

void DasherTest::setUp()
{
    dasher = new Dasher();
    dasher->configure(dashFolder, baseName, SEG_DURATION);
    h264Filter = new VideoFilterMockup(H264);
    vp8Filter = new VideoFilterMockup(VP8);
    aacFilter = new AudioFilterMockup(AAC);
    mp3Filter = new AudioFilterMockup(MP3);
    h264Filter->connectOneToMany(dasher, h264ReaderId);
    vp8Filter->connectOneToMany(dasher, vp8ReaderId);
    aacFilter->connectOneToMany(dasher, aacReaderId);
    mp3Filter->connectOneToMany(dasher, mp3ReaderId);
}

void DasherTest::tearDown()
{
    delete dasher;
    delete h264Filter;
    delete vp8Filter;
    delete aacFilter;
    delete mp3Filter;
}

void DasherTest::configure()
{
    Dasher* tmpDasher;
    std::string invalidFolder("nonExistance");
    
    tmpDasher = new Dasher();
    CPPUNIT_ASSERT(!tmpDasher->configure(invalidFolder, baseName, SEG_DURATION));

    tmpDasher = new Dasher();
    CPPUNIT_ASSERT(!tmpDasher->configure(dashFolder, "", SEG_DURATION));

    tmpDasher = new Dasher();
    CPPUNIT_ASSERT(!tmpDasher->configure(dashFolder, baseName, 0));
    
    tmpDasher = new Dasher();
    CPPUNIT_ASSERT(tmpDasher->configure(dashFolder, baseName, SEG_DURATION));
    
    delete tmpDasher;
}

void DasherTest::addSegmenter()
{
    CPPUNIT_ASSERT(!dasher->addSegmenter(nonExistanceReader));
    CPPUNIT_ASSERT(!dasher->addSegmenter(vp8ReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(mp3ReaderId));
    CPPUNIT_ASSERT(dasher->addSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(dasher->addSegmenter(aacReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(!dasher->addSegmenter(aacReaderId));
}

void DasherTest::removeSegmenter()
{
    dasher->addSegmenter(h264ReaderId);
    dasher->addSegmenter(aacReaderId);
    CPPUNIT_ASSERT(dasher->removeSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(dasher->removeSegmenter(aacReaderId));
    CPPUNIT_ASSERT(!dasher->removeSegmenter(h264ReaderId));
    CPPUNIT_ASSERT(!dasher->removeSegmenter(aacReaderId));
}

void DasherTest::setDashSegmenterBitrate() 
{
    dasher->addSegmenter(h264ReaderId);
    CPPUNIT_ASSERT(dasher->setDashSegmenterBitrate(h264ReaderId, 1000000));
    CPPUNIT_ASSERT(!dasher->setDashSegmenterBitrate(aacReaderId, 1000000));
}

CPPUNIT_TEST_SUITE_REGISTRATION(DasherTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("DasherTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
