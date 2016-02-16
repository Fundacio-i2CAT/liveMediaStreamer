/*
 *  IOInterfaceTest - IOInterfaceTest classes test
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
 *  Authors:    David Cassany <david.cassany@i2cat.net>
 *
 */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "IOInterface.hh"
#include "Utils.hh"
#include "AVFramedQueueMockup.hh"

#define TIME_WAIT 100

class IOInterfaceTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(IOInterfaceTest);
    CPPUNIT_TEST(readerTest);
    CPPUNIT_TEST(setConnectionTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void readerTest();
    void setConnectionTest();
    
private:
    Reader *reader;
    Writer *writer;
    FrameQueue *queue;
};

void IOInterfaceTest::setUp() 
{
    reader = new Reader();
    ConnectionData cData;
    ReaderData reader;
    StreamInfo si;
    
    reader.rFilterId = 2;
    reader.readerId = 2;
    
    cData.wFilterId = 1;
    cData.writerId = 1;
    cData.readers.push_back(reader);
    
    queue = new AVFramedQueueMock(cData, &si, 8);
    CPPUNIT_ASSERT(queue != NULL);
}

void IOInterfaceTest::tearDown()
{
    queue->setConnected(false);
    delete reader;
}

void IOInterfaceTest::setConnectionTest()
{
    CPPUNIT_ASSERT(!reader->isConnected());
    reader->setConnection(NULL);
    CPPUNIT_ASSERT(!reader->isConnected());
    reader->setConnection(queue);
    CPPUNIT_ASSERT(!reader->isConnected());
    queue->setConnected(true);
    CPPUNIT_ASSERT(queue->isConnected());
    CPPUNIT_ASSERT(reader->isConnected());
    reader->setConnection(NULL);
    CPPUNIT_ASSERT(reader->isConnected());
}

void IOInterfaceTest::readerTest()
{
    bool gotFrame;
    reader->setConnection(queue); 
    queue->setConnected(true);
    CPPUNIT_ASSERT(queue->isConnected());
    CPPUNIT_ASSERT(reader->isConnected());
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    CPPUNIT_ASSERT(reader->getFrame(2, gotFrame) != NULL);
    CPPUNIT_ASSERT(gotFrame == false);
    
    queue->addFrame();
    queue->addFrame();
    queue->addFrame();
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);

    queue->removeFrame();
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->addReader(3, 3);
    queue->addFrame();
    queue->addFrame();
    queue->addFrame();
    queue->addFrame();
    
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(2);
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(3);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->addReader(4, 4);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(2);
    reader->removeFrame(3);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->removeFrame(2);
    reader->removeFrame(3);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeFrame(4);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->getFrame(4, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->removeFrame(4);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(4, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->removeFrame(2);
    reader->removeFrame(3);
    reader->removeFrame(4);
    
    reader->removeReader(4);
    
    queue->addFrame();
    queue->addFrame();
    queue->addFrame();
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(3, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->removeReader(3);
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->removeFrame(2);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == true);
    
    reader->getFrame(2, gotFrame);
    CPPUNIT_ASSERT(gotFrame == false);
    
    reader->removeReader(2);
    
    CPPUNIT_ASSERT(!reader->isConnected());
    CPPUNIT_ASSERT(!queue->isConnected());
}

CPPUNIT_TEST_SUITE_REGISTRATION(IOInterfaceTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("IOInterfaceTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();
    
    utils::printMood(runner.result().wasSuccessful());
    
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}