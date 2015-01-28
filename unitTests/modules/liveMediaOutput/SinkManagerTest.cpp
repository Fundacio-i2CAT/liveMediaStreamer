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

#include <GroupsockHelper.hh>

#include "SinkManager.hh"
#include "../../AVFramedQueue.hh"
#include "../../AudioCircularBuffer.hh"
#include "H264QueueServerMediaSubsession.hh"
#include "VP8QueueServerMediaSubsession.hh"
#include "AudioQueueServerMediaSubsession.hh"
#include "QueueSource.hh"
#include "H264QueueSource.hh"
#include "Types.hh"
#include "Connection.hh"
#include "../../Types.hh"
#include "../../Utils.hh"

class SinkManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SinkManagerTest);
    CPPUNIT_TEST(addRTPConnection);
    CPPUNIT_TEST(addMpegTsRTPConnection);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addRTPConnection();
    void addMpegTsRTPConnection();

protected:
    SinkManager* sinkManager = NULL;
    std::stringstream voidstream;
};

void SinkManagerTest::setUp()
{
	sinkManager = sinkManager->getInstance();
    std::cerr.rdbuf(voidstream.rdbuf());
}

void SinkManagerTest::tearDown()
{
    delete sinkManager;
}

void SinkManagerTest::addMpegTsRTPConnection()
{
    CPPUNIT_ASSERT(sinkManager != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION( SinkManagerTest );

int main(int argc, char* argv[])
{
    std::ofstream xmlout("SinkManagerTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
}
