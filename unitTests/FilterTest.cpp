/*
 *  FilterTest.cpp - Filter class test
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
 *              Gerard Castillo <gerard.castillo@i2cat.net>
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
#include "WorkersPool.hh"

class FilterUnitTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(FilterUnitTest);
    CPPUNIT_TEST(connectOneToOne);
    CPPUNIT_TEST(connectManyToOne);
    CPPUNIT_TEST(connectOneToMany);
    CPPUNIT_TEST(connectManyToMany);
    CPPUNIT_TEST(shareReader);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void connectOneToOne();
    void connectManyToOne();
    void connectOneToMany();
    void connectManyToMany();
    void shareReader();
};

void FilterUnitTest::setUp()
{

}

void FilterUnitTest::tearDown()
{

}

void FilterUnitTest::connectOneToOne()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter = new BaseFilterMockup(1,1);

    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(!satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilter));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));

    delete filterToTest;
    delete satelliteFilter;
}

void FilterUnitTest::connectManyToOne()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,2);
    BaseFilter* satelliteFilter = new BaseFilterMockup(1,1);

    CPPUNIT_ASSERT(filterToTest->connectManyToOne(satelliteFilter, 1));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 3));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));

    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(2));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(1));

    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->connectManyToOne(satelliteFilter, 2));

    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(3));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));

    delete filterToTest;
    delete satelliteFilter;
}

void FilterUnitTest::connectOneToMany()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter = new BaseFilterMockup(2,1);

    CPPUNIT_ASSERT(filterToTest->connectOneToMany(satelliteFilter,1));
    CPPUNIT_ASSERT(!filterToTest->connectOneToMany(satelliteFilter,1));
    CPPUNIT_ASSERT(!filterToTest->connectOneToMany(satelliteFilter,2));

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(filterToTest->connectOneToMany(satelliteFilter,2));

    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(2));

    delete filterToTest;
    delete satelliteFilter;
}

void FilterUnitTest::connectManyToMany()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,2);
    BaseFilter* satelliteFilter = new BaseFilterMockup(2,1);

    CPPUNIT_ASSERT(filterToTest->connectManyToMany(satelliteFilter,1,1));
    CPPUNIT_ASSERT(filterToTest->connectManyToMany(satelliteFilter,2,2));

    CPPUNIT_ASSERT(!filterToTest->connectManyToMany(satelliteFilter,2,1));
    CPPUNIT_ASSERT(!filterToTest->connectManyToMany(satelliteFilter,1,2));

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(2));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(2));

    CPPUNIT_ASSERT(filterToTest->connectManyToMany(satelliteFilter,2,1));
    CPPUNIT_ASSERT(filterToTest->connectManyToMany(satelliteFilter,1,2));

    filterToTest->disconnectAll();
    satelliteFilter->disconnectAll();

    delete filterToTest;
    delete satelliteFilter;
}

void FilterUnitTest::shareReader()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter2 = new BaseFilterMockup(1,1);

    CPPUNIT_ASSERT(!satelliteFilter->shareReader(satelliteFilter2, 1, 1));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilter));
    
    CPPUNIT_ASSERT(satelliteFilter->shareReader(satelliteFilter2, 1, 1));
    CPPUNIT_ASSERT(!satelliteFilter->shareReader(satelliteFilter2, 1, 1));
    
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));

    delete filterToTest;
    delete satelliteFilter;
    delete satelliteFilter2;
}

class FilterFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(FilterFunctionalTest);
    CPPUNIT_TEST(oneToOneMasterProcessFrame);
    CPPUNIT_TEST(oneToOneSlaveProcessFrame);
    CPPUNIT_TEST(masterSlavesSharedFramesTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:

    void oneToOneMasterProcessFrame();
    void oneToOneSlaveProcessFrame();
    void masterSlavesSharedFramesTest();
};

void FilterFunctionalTest::setUp()
{

}

void FilterFunctionalTest::tearDown()
{

}

void FilterFunctionalTest::oneToOneMasterProcessFrame()
{
//     std::chrono::microseconds frameTime = std::chrono::microseconds(15000);
//     std::chrono::microseconds processTime = std::chrono::microseconds(10000);
// 
//     //TODO: this test should be done with HeadFilterMockup
//     BaseFilter* filterToTest = new OneToOneFilterMockup(processTime, 4, true, frameTime, MASTER);
//     BaseFilter* satelliteFilterFirst = new BaseFilterMockup(1,1);
//     BaseFilter* satelliteFilterLast = new BaseFilterMockup(1,1);
// 
//     OneToOneFilterMockup* filterMockup;
//     Reader* reader;
//     AVFramedQueueMock* frameQueue;
//     int time;
//     int ret;
// 
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     filterMockup = dynamic_cast<OneToOneFilterMockup*>(filterToTest);
// 
//     CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
//     CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
// 
//     reader = filterMockup->getReader(1);
//     frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     CPPUNIT_TEST_SUITE(FilterFunctionalTest);
//     CPPUNIT_TEST(oneToOneMasterProcessFrame);
//     CPPUNIT_TEST(oneToOneSlaveProcessFrame);
//     CPPUNIT_TEST(masterSlavesSharedFramesTest);
//     CPPUNIT_TEST_SUITE_END();    for (int i = 0; i < 10; i++){
//         frameQueue->addFrame();
//         filterToTest->processFrame(ret);
//         time = ret;
//         utils::debugMsg("Time to sleep in microseconds " + std::to_string(time));
//         CPPUNIT_ASSERT(time > 0 && time >= processTime.count()/2 && time < frameTime.count()*1.5);
//     }
// 
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     delete filterToTest;
//     CPPUNIT_ASSERT(satelliteFilterFirst->disconnectWriter(1));
//     CPPUNIT_ASSERT(satelliteFilterLast->disconnectReader(1));
// 
//     filterToTest = new OneToOneFilterMockup(processTime, 4, true, std::chrono::microseconds(0), MASTER);
//     filterMockup = dynamic_cast<OneToOneFilterMockup*>(filterToTest);
// 
//     CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
//     CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
// 
//     reader = filterMockup->getReader(1);
//     CPPUNIT_ASSERT(reader != NULL);
//     frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     frameQueue->addFrame();
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == 0);
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     delete filterToTest;
//     delete satelliteFilterFirst;
//     delete satelliteFilterLast;
}

void FilterFunctionalTest::oneToOneSlaveProcessFrame()
{
//     std::chrono::microseconds frameTime = std::chrono::microseconds(15000);
//     std::chrono::microseconds processTime = std::chrono::microseconds(10000);
//     int ret;
//     
//     BaseFilter* filterToTest = new OneToOneFilterMockup(processTime, 4, true, frameTime, SLAVE);
//     BaseFilter* satelliteFilterFirst = new BaseFilterMockup(2,1);
//     BaseFilter* satelliteFilterLast = new BaseFilterMockup(1,2);
// 
//     Reader* reader;
//     AVFramedQueueMock* frameQueue;
// 
//     CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
//     CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
// 
//     reader = (dynamic_cast<OneToOneFilterMockup*>(filterToTest))->getReader(1);
//     CPPUNIT_ASSERT(reader != NULL);
//     frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     frameQueue->addFrame();
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     frameQueue->addFrame();
//     frameQueue->addFrame();
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     delete filterToTest;
//     CPPUNIT_ASSERT(satelliteFilterFirst->disconnectWriter(1));
//     CPPUNIT_ASSERT(satelliteFilterLast->disconnectReader(1));
// 
//     filterToTest = new OneToOneFilterMockup(processTime, 4, false, std::chrono::microseconds(0), SLAVE);
// 
//     CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
//     CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
// 
//     reader = (dynamic_cast<OneToOneFilterMockup*>(filterToTest))->getReader(1);
//     CPPUNIT_ASSERT(reader != NULL);
//     frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     frameQueue->addFrame();
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
//     filterToTest->processFrame(ret);
//     CPPUNIT_ASSERT(ret == RETRY);
// 
//     delete filterToTest;
//     delete satelliteFilterFirst;
//     delete satelliteFilterLast;
}

void FilterFunctionalTest::masterSlavesSharedFramesTest()
{
//     std::chrono::microseconds frameTime = std::chrono::microseconds(40000);
//     std::chrono::microseconds processTime = std::chrono::microseconds(15000);
//     std::cout << "slaves test" << std::endl;
//     TODO: we should recheck sharedFrames slaves doesn't have a connected reader
//     BaseFilter* master = new OneToOneFilterMockup(processTime, 4, true, frameTime, MASTER);
//     BaseFilter* slave1 = new OneToOneFilterMockup(processTime, 4, true, frameTime, SLAVE);
//     BaseFilter* slave2 = new OneToOneFilterMockup(processTime, 4, false, frameTime, SLAVE);
//     BaseFilter* fakeSlave = new OneToOneFilterMockup(processTime, 4, true, frameTime, MASTER);
//     
//     WorkersPool* pool = new WorkersPool();
//     
//     master->setId(1);
//     slave1->setId(2);
//     slave2->setId(3);
//     fakeSlave->setId(4);
// 
//     TODO: they  should be head/tail filters mockup
//     BaseFilter* satelliteFilterFirst = new BaseFilterMockup(1,1);
//     BaseFilter* satelliteFilterLast = new BaseFilterMockup(3,1);
// 
//     Reader* reader;
//     AVFramedQueueMock *masterIn, *slave1Out, *slave2Out;
// 
//     CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(master));
//     CPPUNIT_ASSERT(master->connectOneToMany(satelliteFilterLast, 1));
//     CPPUNIT_ASSERT(slave1->connectOneToMany(satelliteFilterLast, 2));
//     CPPUNIT_ASSERT(slave2->connectOneToMany(satelliteFilterLast, 3));
// 
//     TODO: we must rethink masters & salves relationship
//     CPPUNIT_ASSERT(master->addSlave(slave1));
//     CPPUNIT_ASSERT(!master->addSlave(slave1));
//     CPPUNIT_ASSERT(master->addSlave(slave2));
//     CPPUNIT_ASSERT(!master->addSlave(slave2));
//     CPPUNIT_ASSERT(!master->addSlave(fakeSlave));
// 
//     reader = (dynamic_cast<OneToOneFilterMockup*>(master))->getReader(1);
//     CPPUNIT_ASSERT(reader != NULL);
//     masterIn = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(2);
//     CPPUNIT_ASSERT(reader != NULL);
//     slave1Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(3);
//     CPPUNIT_ASSERT(reader != NULL);
//     slave2Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
// 
//     CPPUNIT_ASSERT(masterIn->getElements() == 0);
//     CPPUNIT_ASSERT(slave1Out->getElements() == 0);
//     CPPUNIT_ASSERT(slave2Out->getElements() == 0);
// 
//     masterIn->addFrame();
//     masterIn->addFrame();
//     masterIn->addFrame();
// 
//     CPPUNIT_ASSERT(masterIn->getElements() == 3);
//     
//     CPPUNIT_ASSERT(pool->addTask(master));
//     CPPUNIT_ASSERT(pool->addTask(slave1));
//     CPPUNIT_ASSERT(pool->addTask(slave2));
// 
//     std::this_thread::sleep_for(std::chrono::seconds(1));
// 
//     CPPUNIT_ASSERT(masterIn->getElements() == 0);
//     CPPUNIT_ASSERT(slave1Out->getElements() == 3);
//     CPPUNIT_ASSERT(slave2Out->getElements() == 0);
//     
//     delete pool;
//     
//     size_t seq = 0;
//     size_t elements = slave1Out->getElements();
// 
//     for (size_t i = 0; i < elements; i ++){
//         CPPUNIT_ASSERT(seq < slave1Out->getFront()->getSequenceNumber());
//         seq = slave1Out->getFront()->getSequenceNumber();
//         slave1Out->removeFrame();
//     }
// 
//     delete master;
//     delete slave1;
//     delete slave2;
//     delete satelliteFilterFirst;
//     delete satelliteFilterLast;
//     delete fakeSlave;
}

CPPUNIT_TEST_SUITE_REGISTRATION(FilterFunctionalTest);
CPPUNIT_TEST_SUITE_REGISTRATION(FilterUnitTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("FilterTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();
    
    utils::printMood(runner.result().wasSuccessful());
    
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}
