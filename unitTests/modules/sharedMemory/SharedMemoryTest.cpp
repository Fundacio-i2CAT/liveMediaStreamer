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
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "FilterFunctionalMockup.hh"
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

    BaseFilter* sharedMemoryFilter;
    BaseFilter* sharedMemoryFilterErr;
    BaseFilter* satelliteFilterHead;
    BaseFilter* satelliteFilterTail;
};

void SharedMemoryTest::setUp()
{
    CPPUNIT_ASSERT ((sharedMemoryFilter = SharedMemory::createNew(KEY, RAW)) != NULL);
    CPPUNIT_ASSERT ((sharedMemoryFilterErr = SharedMemory::createNew(KEY, RAW)) == NULL);
    satelliteFilterHead = new BaseFilterMockup(0,1);
    satelliteFilterTail = new BaseFilterMockup(1,0);
}

void SharedMemoryTest::tearDown()
{
    if (sharedMemoryFilter) {
        sharedMemoryFilter->disconnectAll();
        delete sharedMemoryFilter;
        sharedMemoryFilter = NULL;
    }
    delete satelliteFilterHead;
    delete satelliteFilterTail;
}

void SharedMemoryTest::connectWithSharedMemory()
{
    CPPUNIT_ASSERT(satelliteFilterHead->connectOneToOne(sharedMemoryFilter));
    CPPUNIT_ASSERT(sharedMemoryFilter->connectOneToOne(satelliteFilterTail));

    CPPUNIT_ASSERT(sharedMemoryFilter->disconnectWriter(1));
    CPPUNIT_ASSERT(sharedMemoryFilter->disconnectReader(1));
    CPPUNIT_ASSERT(satelliteFilterHead->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilterTail->disconnectReader(1));
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

    OneToOneVideoScenarioMockup *sharedMemorySce;
    AVFramesReader* reader;
    SharedMemory* sharedMemoryFilter;
    SharedMemoryDummyReader* dummyReader;
};

void SharedMemoryFunctionalTest::setUp()
{
    sharedMemoryFilter = SharedMemory::createNew(KEY, RAW);
    if (!sharedMemoryFilter) return;
    dummyReader = new SharedMemoryDummyReader((dynamic_cast<SharedMemory*>(sharedMemoryFilter))->getSharedMemoryID(), RAW);
    sharedMemorySce = new OneToOneVideoScenarioMockup((dynamic_cast<OneToOneFilter*>(sharedMemoryFilter)), RAW, YUV420P);
    CPPUNIT_ASSERT(sharedMemorySce->connectFilter());
    reader = new AVFramesReader();
}

void SharedMemoryFunctionalTest::tearDown()
{
    if (sharedMemoryFilter) {
        sharedMemoryFilter->disconnectAll();
        delete sharedMemoryFilter;
    }
    if (sharedMemorySce) {
        sharedMemorySce->disconnectFilter();
    }
    if (reader) {
        delete reader;
    }
}

void SharedMemoryFunctionalTest::sharedMemoryFilterWithDummyReader()
{
    InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame *midFrame = NULL;

    CPPUNIT_ASSERT(reader != NULL);

    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest.h264", RAW, YUV420P, 1280, 720));

    while((frame = reader->getFrame())!=NULL){
        sharedMemorySce->processFrame(frame);
        while ((midFrame = sharedMemorySce->extractFrame())){
            CPPUNIT_ASSERT(memcmp(midFrame->getDataBuf(), frame->getDataBuf(), midFrame->getLength()) == 0);
            CPPUNIT_ASSERT(dummyReader->isEnabled());
            CPPUNIT_ASSERT(dummyReader->isReadable());
            dummyReader->readFramePayload();
            CPPUNIT_ASSERT(memcmp(frame->getDataBuf(), dummyReader->readSharedFrame(), dummyReader->getFrameObject()->getLength()) == 0);
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getWidth() == (dynamic_cast<VideoFrame*>(frame))->getWidth());
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getPixelFormat() == (dynamic_cast<VideoFrame*>(frame))->getPixelFormat());
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getCodec() == (dynamic_cast<VideoFrame*>(frame))->getCodec());
        }
    }
    
    reader->close();

    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest.h264", H264));

    while((frame = reader->getFrame())!=NULL){
        sharedMemorySce->processFrame(frame);
        while ((midFrame = sharedMemorySce->extractFrame())){
            CPPUNIT_ASSERT(memcmp(midFrame->getDataBuf(), frame->getDataBuf(), midFrame->getLength()) == 0);
            CPPUNIT_ASSERT(dummyReader->isEnabled());
            CPPUNIT_ASSERT(dummyReader->isReadable());
            dummyReader->readFramePayload();
            CPPUNIT_ASSERT(memcmp(frame->getDataBuf(), dummyReader->readSharedFrame(), dummyReader->getFrameObject()->getLength()) == 0);
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getWidth() == (dynamic_cast<VideoFrame*>(frame))->getWidth());
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getPixelFormat() == (dynamic_cast<VideoFrame*>(frame))->getPixelFormat());
            CPPUNIT_ASSERT((dynamic_cast<VideoFrame*>(dummyReader->getFrameObject()))->getCodec() == (dynamic_cast<VideoFrame*>(frame))->getCodec());        }
    }
    
    reader->close();

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

    utils::printMood(runner.result().wasSuccessful());
    return runner.result().wasSuccessful() ? 0 : 1;
}
