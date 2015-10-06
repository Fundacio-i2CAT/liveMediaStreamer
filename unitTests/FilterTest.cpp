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

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
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

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(2));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));

    CPPUNIT_ASSERT(!filterToTest->connectManyToOne(satelliteFilter, 2));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->connectManyToOne(satelliteFilter, 2));

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(3));
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

    CPPUNIT_ASSERT(filterToTest->disconnectWriter(2));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(1));
    CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
    CPPUNIT_ASSERT(satelliteFilter->disconnectReader(2));

    delete filterToTest;
    delete satelliteFilter;
}

void FilterUnitTest::shareReader()
{
    BaseFilter* filterToTest = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter = new BaseFilterMockup(1,1);
    BaseFilter* satelliteFilter2 = new BaseFilterMockup(1,1);
    
    filterToTest->setId(0);
    satelliteFilter->setId(1);
    satelliteFilter2->setId(2);

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
    CPPUNIT_TEST(functionalTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:

    void functionalTest();
};

void FilterFunctionalTest::setUp()
{

}

void FilterFunctionalTest::tearDown()
{

}

void FilterFunctionalTest::functionalTest()
{

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
