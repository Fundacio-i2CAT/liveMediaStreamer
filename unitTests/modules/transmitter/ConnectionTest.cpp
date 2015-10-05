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
 *            David Cassany <david.cassany@i2cat.net>
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

#include "modules/transmitter/Connection.hh"
#include "Utils.hh"

#define CHANNELS 2
#define SAMPLERATE 48000
#define RTSP_PORT 8554

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

class RTSPConnectionTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(RTSPConnectionTest);
    CPPUNIT_TEST(addAudioMPEGTS);
    CPPUNIT_TEST(addVideoMPEGTS);
    CPPUNIT_TEST(addAudioAndVideoMPEGTS);
    CPPUNIT_TEST(addAudioSTD);
    CPPUNIT_TEST(addVideoSTD);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addAudioMPEGTS();
    void addVideoMPEGTS();
    void addAudioAndVideoMPEGTS();
    void addAudioSTD();
    void addVideoSTD();

private:
    std::string name = "RTSPTest";
    RTSPConnection* conn;
    
    RTSPServer* rtspServer;
    TaskScheduler* scheduler;
    UsageEnvironment* env;
    FramedSource *vInputSource;
    FramedSource *aInputSource;
    H264VideoStreamFramer* framer;
    StreamReplicator* aReplicator;
    StreamReplicator* vReplicator;
    
    std::string vInputFileName = "testsData/modules/liveMediaOutput/connectionTest/mpegTsConnectionTestVideoInputFile.h264";
    std::string aInputFileName = "testsData/modules/liveMediaOutput/connectionTest/mpegTsConnectionTestAudioInputFile.aac";
};

void RTSPConnectionTest::setUp()
{
    std::ifstream f(vInputFileName.c_str());
    CPPUNIT_ASSERT(f.good());
    
    std::ifstream g(aInputFileName.c_str());
    CPPUNIT_ASSERT(g.good());
    
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    vInputSource = ByteStreamFileSource::createNew(*env, vInputFileName.c_str());
    framer = H264VideoStreamFramer::createNew(*env, vInputSource, True/*includeStartCodeInOutput*/);
    aInputSource = ADTSAudioFileSource::createNew(*env, aInputFileName.c_str());
    
    vReplicator = StreamReplicator::createNew(*env, vInputSource, False);
    aReplicator = StreamReplicator::createNew(*env, aInputSource, False);
    
    CPPUNIT_ASSERT((rtspServer = RTSPServer::createNew(*env, RTSP_PORT, NULL)) != NULL);
}

void RTSPConnectionTest::tearDown()
{
    if (conn){
        delete conn;
    }
    Medium::close(rtspServer);
}

void RTSPConnectionTest::addAudioSTD()
{
    conn = new RTSPConnection(env, STD_RTP, rtspServer, name);

    CPPUNIT_ASSERT(conn != NULL);

    CPPUNIT_ASSERT(!conn->addAudioSubsession(AAC, NULL, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(conn->addAudioSubsession(AAC, aReplicator, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(!conn->addAudioSubsession(AAC, aReplicator, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(conn->addAudioSubsession(MP3, aReplicator, CHANNELS, SAMPLERATE, S16, 2));

    CPPUNIT_ASSERT(conn->setup());
}

void RTSPConnectionTest::addVideoSTD()
{
    conn = new RTSPConnection(env, STD_RTP, rtspServer, name);
    
    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(framer != NULL);

    CPPUNIT_ASSERT(!conn->addVideoSubsession(H264, NULL, 1));
    CPPUNIT_ASSERT(conn->addVideoSubsession(H265, vReplicator, 1));
    CPPUNIT_ASSERT(!conn->addVideoSubsession(H264, vReplicator, 1));
    CPPUNIT_ASSERT(conn->addVideoSubsession(H264, vReplicator, 2));
    
    CPPUNIT_ASSERT(conn->setup());
}

void RTSPConnectionTest::addAudioMPEGTS()
{
    conn = new RTSPConnection(env, MPEGTS, rtspServer, name);
    
    CPPUNIT_ASSERT(conn != NULL);

    CPPUNIT_ASSERT(!conn->addAudioSubsession(AAC, NULL, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(conn->addAudioSubsession(MP3, aReplicator, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(!conn->addAudioSubsession(AAC, aReplicator, CHANNELS, SAMPLERATE, S16, 1));

    CPPUNIT_ASSERT(conn->setup());
}

void RTSPConnectionTest::addVideoMPEGTS()
{
    conn = new RTSPConnection(env, MPEGTS, rtspServer, name);
    
    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(framer != NULL);

    CPPUNIT_ASSERT(!conn->addVideoSubsession(H264, NULL, 1));
    CPPUNIT_ASSERT(!conn->addVideoSubsession(VP8, vReplicator, 1));
    CPPUNIT_ASSERT(conn->addVideoSubsession(H264, vReplicator, 1));

    CPPUNIT_ASSERT(conn->setup());
}

void RTSPConnectionTest::addAudioAndVideoMPEGTS()
{
    conn = new RTSPConnection(env, MPEGTS, rtspServer, name);
    
    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(framer != NULL);
    
    CPPUNIT_ASSERT(conn->addAudioSubsession(AAC, aReplicator, CHANNELS, SAMPLERATE, S16, 1));
    CPPUNIT_ASSERT(!conn->addVideoSubsession(H264, vReplicator, 1));
    CPPUNIT_ASSERT(conn->addVideoSubsession(H264, vReplicator, 2));
    
    CPPUNIT_ASSERT(conn->setup());
}

void MpegTsConnectionTest::setUp()
{
    std::ifstream f(vInputFileName.c_str());
    CPPUNIT_ASSERT(f.good());
    
    std::ifstream g(aInputFileName.c_str());
    CPPUNIT_ASSERT(g.good());
    
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
    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(framer != NULL);

    CPPUNIT_ASSERT(!conn->addVideoSource(NULL, H264, 1));
    CPPUNIT_ASSERT(!conn->addVideoSource(framer, VP8, 1));
    CPPUNIT_ASSERT(conn->addVideoSource(framer, H264, 1));

    CPPUNIT_ASSERT(conn->setup());
}

void MpegTsConnectionTest::addAudioSource()
{
    CPPUNIT_ASSERT(conn != NULL);
    CPPUNIT_ASSERT(aInputSource != NULL);

    CPPUNIT_ASSERT(!conn->addAudioSource(NULL, AAC, 2));
    CPPUNIT_ASSERT(conn->addAudioSource(aInputSource, MP3, 2));
    CPPUNIT_ASSERT(!conn->addAudioSource(aInputSource, AAC, 2));
    
    CPPUNIT_ASSERT(conn->setup());
}

CPPUNIT_TEST_SUITE_REGISTRATION(MpegTsConnectionTest);
CPPUNIT_TEST_SUITE_REGISTRATION(RTSPConnectionTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("MpegTsConnectionTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();
    
    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
} 
