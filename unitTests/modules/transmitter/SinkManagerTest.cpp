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
 *  Authors:  Gerard Castillo <gerard.castillo@i2cat.net>,
 *            Marc Palau <marc.palau@i2cat.net>
 *
 */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "modules/transmitter/SinkManager.hh"
#include "../../FilterMockup.hh"

class SinkManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SinkManagerTest);
    CPPUNIT_TEST(addMpegTsRTPConnection);
    CPPUNIT_TEST(addRTSPConnectionMPEGTS);
    CPPUNIT_TEST(addRTSPConnectionSTD);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addMpegTsRTPConnection();
    void addRTSPConnectionMPEGTS();
    void addRTSPConnectionSTD();

protected:
    SinkManager* sinkManager = NULL;
    VideoFilterMockup* vFilter = NULL;
    AudioFilterMockup* aFilter = NULL;
    int vReaderId = 234;
    int aReaderId = 236;
    int fakevReaderId = vReaderId + 1;
    int fakeaReaderId = aReaderId + 1;
};

void SinkManagerTest::setUp()
{
    sinkManager = SinkManager::createNew();
    vFilter = new VideoFilterMockup(H264);
    aFilter = new AudioFilterMockup(AAC);
    vFilter->connectOneToMany(sinkManager, vReaderId);
    aFilter->connectOneToMany(sinkManager, aReaderId);
}

void SinkManagerTest::tearDown()
{
    delete vFilter;
    delete aFilter;
    delete sinkManager;
}

void SinkManagerTest::addMpegTsRTPConnection()
{
    std::vector<int> readers;
    int id = 1234;
    std::string ip = "127.0.0.1";
    int port = 6004;
    TxFormat txFormat = MPEGTS;

    CPPUNIT_ASSERT(sinkManager != NULL);
    CPPUNIT_ASSERT(vFilter != NULL);

    readers.push_back(fakevReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTPConnection(readers, id, ip, port, txFormat));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(vReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTPConnection(readers, id, ip, port, txFormat));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(aReaderId);
    CPPUNIT_ASSERT(sinkManager->addRTPConnection(readers, id, ip, port, txFormat));

    CPPUNIT_ASSERT(!sinkManager->addRTPConnection(readers, id, ip, port, txFormat));
    
    CPPUNIT_ASSERT(sinkManager->removeConnection(id));
    CPPUNIT_ASSERT(!sinkManager->removeConnection(id));
}

void SinkManagerTest::addRTSPConnectionMPEGTS()
{
    std::vector<int> readers;
    std::string name = "testRTSP";
    int id = 1;
    TxFormat txFormat = MPEGTS;

    CPPUNIT_ASSERT(sinkManager != NULL);
    CPPUNIT_ASSERT(vFilter != NULL);

    readers.push_back(fakevReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(vReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(aReaderId);
    CPPUNIT_ASSERT(sinkManager->addRTSPConnection(readers, id, txFormat, name));

    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    
    CPPUNIT_ASSERT(sinkManager->removeConnection(id));
    CPPUNIT_ASSERT(!sinkManager->removeConnection(id));
}

void SinkManagerTest::addRTSPConnectionSTD()
{
    std::vector<int> readers;
    std::string name = "testRTSP";
    int id = 1;
    TxFormat txFormat = STD_RTP;

    CPPUNIT_ASSERT(sinkManager != NULL);
    CPPUNIT_ASSERT(vFilter != NULL);

    readers.push_back(fakevReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(vReaderId);
    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    readers.clear();

    readers.push_back(vReaderId);
    readers.push_back(aReaderId);
    CPPUNIT_ASSERT(sinkManager->addRTSPConnection(readers, id, txFormat, name));

    CPPUNIT_ASSERT(!sinkManager->addRTSPConnection(readers, id, txFormat, name));
    
    CPPUNIT_ASSERT(sinkManager->removeConnection(id));
    CPPUNIT_ASSERT(!sinkManager->removeConnection(id));
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

    utils::printMood(runner.result().wasSuccessful());
    
    return runner.result().wasSuccessful() ? 0 : 1;
}
