/*
 *  SharedMemoryTest.cpp - SharedMemory test filter class
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
 *  Authors:    Gerard Castillo <gerard.castillo@i2cat.net>
 *
 */

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "FilterMockup.hh"
#include "Worker.hh"
#include "modules/sharedMemory/SharedMemory.hh"
#include "SharedMemoryDummyReader.hh"

class SharedMemoryTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SharedMemoryTest);
    CPPUNIT_TEST(connectWithSharedMemory);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void connectWithSharedMemory();
};

void SharedMemoryTest::setUp()
{

}

void SharedMemoryTest::tearDown()
{

}

void SharedMemoryTest::connectWithSharedMemory()
{
    BaseFilter* sharedMemoryFilter = SharedMemory::createNew(KEY, RAW);
    CPPUNIT_ASSERT(!(sharedMemoryFilter == NULL));

    BaseFilter* satelliteFilterHead = new BaseFilterMockup(0,1);
    BaseFilter* satelliteFilterTail = new BaseFilterMockup(1,0);

    BaseFilter* sharedMemoryFilterErr = SharedMemory::createNew(KEY, RAW);
    CPPUNIT_ASSERT(sharedMemoryFilterErr == NULL);
    delete sharedMemoryFilterErr;

    CPPUNIT_ASSERT(satelliteFilterHead->connectOneToOne(sharedMemoryFilter));
    CPPUNIT_ASSERT(sharedMemoryFilter->connectOneToOne(satelliteFilterTail));

    CPPUNIT_ASSERT(sharedMemoryFilter->disconnectWriter(1));
    CPPUNIT_ASSERT(sharedMemoryFilter->disconnectReader(1));
    CPPUNIT_ASSERT(satelliteFilterHead->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilterTail->disconnectReader(1));

    sharedMemoryFilter->disconnectAll();

    delete sharedMemoryFilter;
    delete satelliteFilterHead;
    delete satelliteFilterTail;
}


class SharedMemoryFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SharedMemoryFunctionalTest);
    CPPUNIT_TEST(sharedMemoryFilterWithDummyReader);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:

    void sharedMemoryFilterWithDummyReader();
};

void SharedMemoryFunctionalTest::setUp()
{

}

void SharedMemoryFunctionalTest::tearDown()
{

}

void SharedMemoryFunctionalTest::sharedMemoryFilterWithDummyReader()
{
    SharedMemoryFilterMockup* sharedMemoryFilter = new SharedMemoryFilterMockup(KEY, RAW);
    CPPUNIT_ASSERT(!(sharedMemoryFilter == NULL));
    BaseFilter* satelliteFilterHead = new BaseFilterMockup(0,1);
    BaseFilter* satelliteFilterTail = new BaseFilterMockup(1,0);
    VideoFrameMock* frameMockup = VideoFrameMock::createNew();
    uint16_t seqNum = 1;
    std::chrono::microseconds fakeTimeStamp;
    std::chrono::system_clock::time_point startPoint;

    BaseFilter* sharedMemoryFilterB = dynamic_cast<BaseFilter*>(sharedMemoryFilter);

    CPPUNIT_ASSERT(sharedMemoryFilterB->processFrame() == RETRY);

    CPPUNIT_ASSERT(satelliteFilterHead->connectOneToOne(sharedMemoryFilter));
    CPPUNIT_ASSERT(sharedMemoryFilter->connectOneToOne(satelliteFilterTail));

    SharedMemoryDummyReader* dummyReader = new SharedMemoryDummyReader(sharedMemoryFilter->getSharedMemoryID(), RAW);

    std::thread dReader(SharedMemoryDummyReader::dummyReaderThread, dummyReader);

    CPPUNIT_ASSERT(sharedMemoryFilter->isEnabled());
    CPPUNIT_ASSERT(sharedMemoryFilter->isWritable());
    //sharedMemoryFilter->setFrameObject(frameMockup);
    startPoint = std::chrono::system_clock::now();

    for(int i = 0; i<10; i++){
        usleep(150000);
        fakeTimeStamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now() - startPoint);
        if(sharedMemoryFilter->isWritable()){
            //sharedMemoryFilter->writeFramePayload(seqNum++);
            //sharedMemoryFilter->getFrameObject()->setPresentationTime(fakeTimeStamp);
            sharedMemoryFilter->writeSharedMemoryRAW(frameMockup->getDataBuf(),frameMockup->getLength());
        }
    }

    dummyReader->setDisabled();
    dReader.join();

    CPPUNIT_ASSERT(dummyReader->getReadFrames() == 10);

    sharedMemoryFilter->disconnectAll();

    delete sharedMemoryFilterB;
    delete dummyReader;
    delete satelliteFilterHead;
    delete satelliteFilterTail;
}

CPPUNIT_TEST_SUITE_REGISTRATION(SharedMemoryFunctionalTest);
CPPUNIT_TEST_SUITE_REGISTRATION(SharedMemoryTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("SharedMemoryTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
}
