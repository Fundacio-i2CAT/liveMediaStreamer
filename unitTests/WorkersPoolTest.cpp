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

#include "WorkersPool.hh"
#include "RunnableMockup.hh"

class WorkersPoolTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(WorkersPoolTest);
    CPPUNIT_TEST(addAndRemoveTask);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void addAndRemoveTask();

private:
    WorkersPool* pool;
};

void WorkersPoolTest::setUp()
{
    pool = new WorkersPool();
}

void WorkersPoolTest::tearDown()
{
    delete pool;
}

void WorkersPoolTest::addAndRemoveTask()
{
    std::vector<int> periodic(1,2);
    std::vector<int> notPeriodic(1,1);
    Runnable* periodicR = new RunnableMockup(40000, periodic, true);
    Runnable* notPeriodicR = new RunnableMockup(40000, notPeriodic, false);
    periodicR->setId(1);
    notPeriodicR->setId(2);
    
    CPPUNIT_ASSERT(pool->addTask(periodicR));
    CPPUNIT_ASSERT(!pool->addTask(periodicR));
    CPPUNIT_ASSERT(!pool->removeTask(2));
    CPPUNIT_ASSERT(pool->removeTask(1));
    CPPUNIT_ASSERT(pool->addTask(periodicR));
    CPPUNIT_ASSERT(pool->addTask(notPeriodicR));
    CPPUNIT_ASSERT(pool->removeTask(1));
    CPPUNIT_ASSERT(pool->removeTask(2));
    CPPUNIT_ASSERT(!pool->removeTask(1));
    CPPUNIT_ASSERT(!pool->removeTask(2));
    
    pool->stop();
    
    delete periodicR;
    delete notPeriodicR;
}

CPPUNIT_TEST_SUITE_REGISTRATION(WorkersPoolTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("WorkersPoolTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    delete outputter;
    
    utils::printMood(runner.result().wasSuccessful());
    return runner.result().wasSuccessful() ? 0 : 1;
} 
