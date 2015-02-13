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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *            
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

class FilterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(FilterTest);
    CPPUNIT_TEST(connectOneToOne);
    CPPUNIT_TEST(connectManyToOne);
    CPPUNIT_TEST(connectOneToMany);
    CPPUNIT_TEST(connectManyToMany);
    CPPUNIT_TEST(oneToOneMasterProcessFrame);
    CPPUNIT_TEST(oneToOneSlaveProcessFrame);
    CPPUNIT_TEST(masterSlavesSharedFramesTest);
    CPPUNIT_TEST(masterSlavesIndependentFramesTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void connectOneToOne();
    void connectManyToOne();
    void connectOneToMany();
    void connectManyToMany();
    void oneToOneMasterProcessFrame();
    void oneToOneSlaveProcessFrame();
    void masterSlavesSharedFramesTest();
    void masterSlavesIndependentFramesTest();
};

void FilterTest::setUp()
{

}

void FilterTest::tearDown()
{

}

void FilterTest::connectOneToOne()
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

void FilterTest::connectManyToOne()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,2);
    BaseFilter* satelliteFilter = new BaseFilterMockup(1,1);
        
    CPPUNIT_ASSERT(filterToTest->connectManyToOne(satelliteFilter, 1));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 3));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
 
    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(2));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->connectManyToOne(satelliteFilter, 2));
    
    CPPUNIT_ASSERT(!filterToTest->disconnectWriter(3));
    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));
    
    delete filterToTest;
    delete satelliteFilter;
}

void FilterTest::connectOneToMany()
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

void FilterTest::connectManyToMany()
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

void FilterTest::oneToOneMasterProcessFrame()
{
    //TODO: this test should be done with HeadFilterMockup
    BaseFilter* filterToTest = new OneToOneFilterMockup(10000,4,false, 15000, MASTER, false);
    BaseFilter* satelliteFilterFirst = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilterLast = new BaseFilterMockup(1,1);
    
    OneToOneFilterMockup* filterMockup;
    Reader* reader;
    AVFramedQueueMock* frameQueue;
    size_t time;
    
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    filterMockup = dynamic_cast<OneToOneFilterMockup*>(filterToTest);
    
    CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
    
    reader = filterMockup->getReader(1);
    frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());

    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    for (int i = 0; i < 10; i++){
        frameQueue->addFrame();
        time = filterToTest->processFrame();
        utils::debugMsg("Time to sleep " + std::to_string(time));
        CPPUNIT_ASSERT(time > 0 && time >= 5000 && time < 11500);
    }
    
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    delete filterToTest;
    CPPUNIT_ASSERT(satelliteFilterFirst->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilterLast->disconnectReader(1));

    filterToTest = new OneToOneFilterMockup(10000,4,false, 0, MASTER, false);
    filterMockup = dynamic_cast<OneToOneFilterMockup*>(filterToTest);
    
    CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
    
    reader = filterMockup->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    frameQueue->addFrame();
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    delete filterToTest;
    delete satelliteFilterFirst;
    delete satelliteFilterLast;
}

void FilterTest::oneToOneSlaveProcessFrame()
{
    BaseFilter* filterToTest = new OneToOneFilterMockup(10000,4,false, 15000, SLAVE, false);
    BaseFilter* satelliteFilterFirst = new BaseFilterMockup(2,1);
    BaseFilter* satelliteFilterLast = new BaseFilterMockup(1,2);
    
    Reader* reader;
    AVFramedQueueMock* frameQueue;
    
    CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
    
    reader = (dynamic_cast<OneToOneFilterMockup*>(filterToTest))->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    frameQueue->addFrame();
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    frameQueue->addFrame();
    frameQueue->addFrame();
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    delete filterToTest;
    CPPUNIT_ASSERT(satelliteFilterFirst->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilterLast->disconnectReader(1));

    filterToTest = new OneToOneFilterMockup(10000,4,false, 0, SLAVE, true);
    
    CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(filterToTest));
    CPPUNIT_ASSERT(filterToTest->connectOneToOne(satelliteFilterLast));
    
    reader = (dynamic_cast<OneToOneFilterMockup*>(filterToTest))->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    frameQueue = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    frameQueue->addFrame();
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    CPPUNIT_ASSERT(filterToTest->processFrame() == RETRY);
    
    delete filterToTest;
    delete satelliteFilterFirst;
    delete satelliteFilterLast;
}

void FilterTest::masterSlavesIndependentFramesTest()
{
    BaseFilter* master = new OneToOneFilterMockup(15000,4,true, 40000, MASTER, false);
    BaseFilter* slave1 = new OneToOneFilterMockup(15000,4,true, 40000, SLAVE, false);
    BaseFilter* slave2 = new OneToOneFilterMockup(15000,4,false, 40000, SLAVE, true);
    BaseFilter* fakeSlave = new OneToOneFilterMockup(15000,4,false, 40000, MASTER, false);
    
    //TODO: they  should be head/tail filters mockup
    BaseFilter* satelliteFilterVeryFirst = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilterFirst = new OneToManyFilterMockup(3, 5000, 4,true, 0, MASTER, false);
    BaseFilter* satelliteFilterLast = new BaseFilterMockup(3,1);
    
    Worker *masterW = new Worker();
    Worker *slaveW1 = new Worker();
    Worker *slaveW2 = new Worker();
    
    Reader* reader;
    AVFramedQueueMock *masterIn, *masterOut, *slave1Out, *slave2Out; 
    
    CPPUNIT_ASSERT(satelliteFilterVeryFirst->connectOneToOne(satelliteFilterFirst));
    CPPUNIT_ASSERT(satelliteFilterFirst->connectManyToOne(master,1));
    CPPUNIT_ASSERT(satelliteFilterFirst->connectManyToOne(slave1,2));
    CPPUNIT_ASSERT(satelliteFilterFirst->connectManyToOne(slave2,3));
    CPPUNIT_ASSERT(master->connectOneToMany(satelliteFilterLast, 1));
    CPPUNIT_ASSERT(slave1->connectOneToMany(satelliteFilterLast, 2));
    CPPUNIT_ASSERT(slave2->connectOneToMany(satelliteFilterLast, 3));
    
    CPPUNIT_ASSERT(master->addSlave(1, slave1));
    CPPUNIT_ASSERT(!master->addSlave(1, slave2));
    CPPUNIT_ASSERT(!master->addSlave(1, slave1));
    CPPUNIT_ASSERT(master->addSlave(2, slave2));
    CPPUNIT_ASSERT(!master->addSlave(3, fakeSlave));
    
    CPPUNIT_ASSERT(masterW->addProcessor(1, master));
    CPPUNIT_ASSERT(masterW->addProcessor(2, satelliteFilterFirst));
    CPPUNIT_ASSERT(slaveW1->addProcessor(1, slave1));
    CPPUNIT_ASSERT(slaveW2->addProcessor(1, slave2));
    
    reader = (dynamic_cast<OneToManyFilterMockup*>(satelliteFilterFirst))->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    masterIn = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    masterOut = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(2);
    CPPUNIT_ASSERT(reader != NULL);
    slave1Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(3);
    CPPUNIT_ASSERT(reader != NULL);
    slave2Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    CPPUNIT_ASSERT(masterIn->getElements() == 0);
    CPPUNIT_ASSERT(masterOut->getElements() == 0);
    CPPUNIT_ASSERT(slave1Out->getElements() == 0);
    CPPUNIT_ASSERT(slave2Out->getElements() == 0);
    
    masterIn->addFrame();
    masterIn->addFrame();
    masterIn->addFrame();
    
    CPPUNIT_ASSERT(masterIn->getElements() == 3);
    
    CPPUNIT_ASSERT(masterW->start());
    CPPUNIT_ASSERT(slaveW1->start());
    CPPUNIT_ASSERT(slaveW2->start());
    
    CPPUNIT_ASSERT(masterW->isRunning());
    CPPUNIT_ASSERT(slaveW1->isRunning());
    CPPUNIT_ASSERT(slaveW2->isRunning());
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    CPPUNIT_ASSERT(masterW->isRunning());
    CPPUNIT_ASSERT(slaveW1->isRunning());
    CPPUNIT_ASSERT(slaveW2->isRunning());
    
    masterW->stop();
    slaveW1->stop();
    slaveW2->stop();
    
    CPPUNIT_ASSERT(!masterW->isRunning());
    CPPUNIT_ASSERT(!slaveW1->isRunning());
    CPPUNIT_ASSERT(!slaveW2->isRunning());
    
    CPPUNIT_ASSERT(masterIn->getElements() == 0);
    
    CPPUNIT_ASSERT(masterOut->getElements() == 3);
    CPPUNIT_ASSERT(slave1Out->getElements() == 3);
    CPPUNIT_ASSERT(slave2Out->getElements() == 0);
    
    delete master;
    delete slave1;
    delete slave2;
    delete satelliteFilterFirst;
    delete satelliteFilterVeryFirst;
    delete satelliteFilterLast;
    delete fakeSlave;
    
    delete masterW;
    delete slaveW1;
    delete slaveW2;
}

void FilterTest::masterSlavesSharedFramesTest()
{
    //TODO: we should recheck sharedFrames slaves doesn't have a connected reader
    BaseFilter* master = new OneToOneFilterMockup(15000,4,false, 40000, MASTER, true);
    BaseFilter* slave1 = new OneToOneFilterMockup(15000,4,true, 40000, SLAVE, false);
    BaseFilter* slave2 = new OneToOneFilterMockup(15000,4,false, 40000, SLAVE, true);
    BaseFilter* fakeSlave = new OneToOneFilterMockup(15000,4,false, 40000, MASTER, false);
    
    //TODO: they  should be head/tail filters mockup
    BaseFilter* satelliteFilterFirst = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilterLast = new BaseFilterMockup(3,1);
    
    Worker *masterW = new Worker();
    Worker *slaveW1 = new Worker();
    Worker *slaveW2 = new Worker();
    
    Reader* reader;
    AVFramedQueueMock *masterIn, *slave1Out, *slave2Out; 
    
    CPPUNIT_ASSERT(satelliteFilterFirst->connectOneToOne(master));
    CPPUNIT_ASSERT(master->connectOneToMany(satelliteFilterLast, 1));
    CPPUNIT_ASSERT(slave1->connectOneToMany(satelliteFilterLast, 2));
    CPPUNIT_ASSERT(slave2->connectOneToMany(satelliteFilterLast, 3));
    
    CPPUNIT_ASSERT(master->addSlave(1, slave1));
    CPPUNIT_ASSERT(!master->addSlave(1, slave2));
    CPPUNIT_ASSERT(!master->addSlave(1, slave1));
    CPPUNIT_ASSERT(master->addSlave(2, slave2));
    CPPUNIT_ASSERT(!master->addSlave(3, fakeSlave));
    
    CPPUNIT_ASSERT(masterW->addProcessor(1, master));
    CPPUNIT_ASSERT(slaveW1->addProcessor(1, slave1));
    CPPUNIT_ASSERT(slaveW2->addProcessor(1, slave2));
    
    reader = (dynamic_cast<OneToOneFilterMockup*>(master))->getReader(1);
    CPPUNIT_ASSERT(reader != NULL);
    masterIn = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(2);
    CPPUNIT_ASSERT(reader != NULL);
    slave1Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    reader = (dynamic_cast<BaseFilterMockup*>(satelliteFilterLast))->getReader(3);
    CPPUNIT_ASSERT(reader != NULL);
    slave2Out = dynamic_cast<AVFramedQueueMock*>(reader->getQueue());
    
    CPPUNIT_ASSERT(masterIn->getElements() == 0);
    CPPUNIT_ASSERT(slave1Out->getElements() == 0);
    CPPUNIT_ASSERT(slave2Out->getElements() == 0);
    
    masterIn->addFrame();
    masterIn->addFrame();
    masterIn->addFrame();
    
    CPPUNIT_ASSERT(masterIn->getElements() == 3);
    
    CPPUNIT_ASSERT(masterW->start());
    CPPUNIT_ASSERT(slaveW1->start());
    CPPUNIT_ASSERT(slaveW2->start());
    
    CPPUNIT_ASSERT(masterW->isRunning());
    CPPUNIT_ASSERT(slaveW1->isRunning());
    CPPUNIT_ASSERT(slaveW2->isRunning());
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    CPPUNIT_ASSERT(masterW->isRunning());
    CPPUNIT_ASSERT(slaveW1->isRunning());
    CPPUNIT_ASSERT(slaveW2->isRunning());
    
    masterW->stop();
    slaveW1->stop();
    slaveW2->stop();
    
    CPPUNIT_ASSERT(!masterW->isRunning());
    CPPUNIT_ASSERT(!slaveW1->isRunning());
    CPPUNIT_ASSERT(!slaveW2->isRunning());
    
    CPPUNIT_ASSERT(masterIn->getElements() == 0);
    
    CPPUNIT_ASSERT(slave1Out->getElements() == 3);
    CPPUNIT_ASSERT(slave2Out->getElements() == 0);
    
    delete master;
    delete slave1;
    delete slave2;
    delete satelliteFilterFirst;
    delete satelliteFilterLast;
    delete fakeSlave;
    
    delete masterW;
    delete slaveW1;
    delete slaveW2;
}

CPPUNIT_TEST_SUITE_REGISTRATION(FilterTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("FilterTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
} 
