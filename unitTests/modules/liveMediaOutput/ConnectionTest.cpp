/*
 *  ConnectionTest.cpp - Connection class test
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

#include "modules/liveMediaOutput/Connection.hh"

class MpegTsConnectionTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(MpegTsConnectionTest);
    CPPUNIT_TEST(addVideoSource);
    CPPUNIT_TEST(addAudioSource);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addVideoSource();
    void addAudioSource();

private:
    std::string ip = "127.0.0.1";
    unsigned port = 6004;
    MpegTsConnection* conn;
    TaskScheduler* scheduler;
    UsageEnvironment* env;
    FramedSource *vInputSource;
    FramedSource *aInputSource;
    H264VideoStreamFramer* framer;
    std::string vInputFileName = "testsData/modules/liveMediaOutput/connectionTest/mpegTsConnectionTestVideoInputFile.h264";
    std::string aInputFileName = "testsData/modules/liveMediaOutput/connectionTest/mpegTsConnectionTestAudioInputFile.aac";
};

void MpegTsConnectionTest::setUp()
{
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    conn = new MpegTsConnection(env, ip, port);
    vInputSource = ByteStreamFileSource::createNew(*env, vInputFileName.c_str());
    framer = H264VideoStreamFramer::createNew(*env, vInputSource, True/*includeStartCodeInOutput*/);
    aInputSource = ADTSAudioFileSource::createNew(*env, aInputFileName.c_str());
}

void MpegTsConnectionTest::tearDown()
{
    delete conn;
}

void MpegTsConnectionTest::addVideoSource()
{
    std::ifstream f(vInputFileName.c_str());
    CPPUNIT_ASSERT(f.good());

    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(framer != NULL);

    CPPUNIT_ASSERT(!conn->addVideoSource(NULL, H264, 1));
    CPPUNIT_ASSERT(!conn->addVideoSource(framer, VP8, 1));
    CPPUNIT_ASSERT(conn->addVideoSource(framer, H264, 1));

    CPPUNIT_ASSERT(conn->setup());
}

void MpegTsConnectionTest::addAudioSource()
{
    std::ifstream f(aInputFileName.c_str());
    CPPUNIT_ASSERT(f.good());

    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(aInputSource != NULL);

    CPPUNIT_ASSERT(!conn->addAudioSource(NULL, AAC, 2));
    CPPUNIT_ASSERT(!conn->addAudioSource(aInputSource, MP3, 2));
    CPPUNIT_ASSERT(conn->addAudioSource(aInputSource, AAC, 2));
    
    CPPUNIT_ASSERT(conn->setup());
}

CPPUNIT_TEST_SUITE_REGISTRATION(MpegTsConnectionTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("MpegTsConnectionTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
} 
