/*
 *  SinkManagerTest.cpp - SinkManager class test
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:  Gerard Castillo <gerard.castillo@i2cat.net>
 *
 *
 */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

class SinkManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SinkManagerTest);
    CPPUNIT_TEST(openCloseInput);
    CPPUNIT_TEST(dumpFormat);
    CPPUNIT_TEST(findStreams);
    CPPUNIT_TEST(readFrame);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void openCloseInput();
    void dumpFormat();
    void findStreams();
    void readFrame();

protected:
    uint64_t startTime;
    Demuxer* demux = NULL;
    std::string filePath = "testData/DemuxerTest_input_data.mp4";
    std::stringstream voidstream;
};
