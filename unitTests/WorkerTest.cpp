/*
 *  WorkerTest.cpp - Worker class test
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

#include "Worker.hh"
#include "Runnable.hh"
#include "RunnableMockup.hh"

class WorkerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(WorkerTest);
    CPPUNIT_TEST(addRemoveProcessor);
    CPPUNIT_TEST(startStopProcessor);
    CPPUNIT_TEST(runningProcessor);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addRemoveProcessor();
    void startStopProcessor();
    void runningProcessor();

private:
    Worker* worker;
};

void WorkerTest::setUp()
{
    worker = new Worker();
}

void WorkerTest::tearDown()
{
    delete worker;
}

void WorkerTest::addRemoveProcessor()
{
    Runnable *run1 = new RunnableMockup(40, 20);
    Runnable *run2 = new RunnableMockup(40, 20);
    
    CPPUNIT_ASSERT(worker->addProcessor(1, run1));
    CPPUNIT_ASSERT(worker->getProcessorsSize()==1);
    CPPUNIT_ASSERT(worker->addProcessor(2, run2));
    CPPUNIT_ASSERT(worker->getProcessorsSize()==2);
    CPPUNIT_ASSERT(!worker->addProcessor(1, run1));
    CPPUNIT_ASSERT(!worker->addProcessor(2, run1));
    CPPUNIT_ASSERT(worker->getProcessorsSize()==2);
    CPPUNIT_ASSERT(worker->removeProcessor(1));
    CPPUNIT_ASSERT(!worker->removeProcessor(1));
    CPPUNIT_ASSERT(worker->getProcessorsSize()==1);
    CPPUNIT_ASSERT(!worker->removeProcessor(3));
    CPPUNIT_ASSERT(worker->getProcessorsSize()==1);
    
    delete run1;
    delete run2;
}

void WorkerTest::startStopProcessor()
{
    CPPUNIT_ASSERT(!worker->isRunning());
    CPPUNIT_ASSERT(worker->start());
    CPPUNIT_ASSERT(worker->isRunning());
    worker->stop();
    CPPUNIT_ASSERT(!worker->isRunning());
}

void WorkerTest::runningProcessor()
{
    Runnable *run1 = new RunnableMockup(40, 20, "one");
    Runnable *run2 = new RunnableMockup(40, 5, "two");
    Runnable *run3 = new RunnableMockup(40, 8, "three");
    Runnable *run4 = new RunnableMockup(40, 10, "four");
    
    CPPUNIT_ASSERT(worker->addProcessor(1, run1));
    CPPUNIT_ASSERT(worker->addProcessor(2, run2));
    
    CPPUNIT_ASSERT(!worker->isRunning());
    CPPUNIT_ASSERT(worker->start());
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
       
    CPPUNIT_ASSERT(worker->addProcessor(3, run3));
    CPPUNIT_ASSERT(worker->addProcessor(4, run4));
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    CPPUNIT_ASSERT(worker->removeProcessor(2));
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    CPPUNIT_ASSERT(worker->isRunning());
    worker->stop();
    CPPUNIT_ASSERT(!worker->isRunning());
    
    delete run1;
    delete run2;
    delete run3;
    delete run4;
}

CPPUNIT_TEST_SUITE_REGISTRATION(WorkerTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("WorkerTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
} 
