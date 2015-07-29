/*
 *  PipelineManagerTest - PipelineManager class test
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

#include "PipelineManager.hh"
#include "FilterMockup.hh"

#define TIME_WAIT 100

class PipelineManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(PipelineManagerTest);
    CPPUNIT_TEST(createAndConnectPath);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void createAndConnectPath();

private:
    PipelineManager *pipe;
};

void PipelineManagerTest::setUp() 
{
    pipe = PipelineManager::getInstance(2);
}

void PipelineManagerTest::tearDown()
{
    PipelineManager::destroyInstance();
}

void PipelineManagerTest::createAndConnectPath()
{
    HeadFilter *head = new HeadFilterMockup();
    TailFilter *tail = new TailFilterMockup();
    OneToOneFilter *midF = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(3, tail));
    CPPUNIT_ASSERT(pipe->addFilter(2, midF));
    
    std::vector<int> mid({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 3, -1, -1, mid));
    CPPUNIT_ASSERT(!pipe->createPath(1, 1, 3, -1, -1, mid));
    
    mid = std::vector<int>({1});
    CPPUNIT_ASSERT(!pipe->createPath(2, 1, 3, -1, -1, mid));
    
    mid = std::vector<int>({3});
    CPPUNIT_ASSERT(!pipe->createPath(3, 1, 3, -1, -1, mid));
    
    mid = std::vector<int>({2,2});
    CPPUNIT_ASSERT(!pipe->createPath(4, 1, 3, -1, -1, mid));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->removePath(1));
    CPPUNIT_ASSERT(!pipe->removePath(2));
}

class PipelineManagerFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(PipelineManagerFunctionalTest);
    CPPUNIT_TEST(lineConnection);
    CPPUNIT_TEST(diamondConnection);
    CPPUNIT_TEST(forkConnectionOrigin);
    CPPUNIT_TEST(forkConnectionEnding);
    CPPUNIT_TEST(forkedDiamondConnectionOrigin);
    CPPUNIT_TEST(forkedDiamondConnectionEnding);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void lineConnection();
    void diamondConnection();
    void forkConnectionOrigin();
    void forkConnectionEnding();
    void forkedDiamondConnectionOrigin();
    void forkedDiamondConnectionEnding();
    
private:
    PipelineManager *pipe;
    Path *first, *second, *third;
};

void PipelineManagerFunctionalTest::setUp() 
{
    pipe = PipelineManager::getInstance(2);
}

void PipelineManagerFunctionalTest::tearDown()
{
    PipelineManager::destroyInstance();
}

void PipelineManagerFunctionalTest::lineConnection()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(3, tail));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    
    std::vector<int> midFilters(1,2);
    
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 3, -1, -1, midFilters));
    CPPUNIT_ASSERT(pipe->connectPath(1));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 1);
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
}

void PipelineManagerFunctionalTest::diamondConnection()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    OneToOneFilter *mid2 = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(4, tail));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    CPPUNIT_ASSERT(pipe->addFilter(3, mid2));  
    
    std::vector<int> midFilters({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 4, 1, -1, midFilters));
    
    std::vector<int> midFilters2({3});
    CPPUNIT_ASSERT(pipe->createPath(2, 1, 4, 2, -1, midFilters2));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->connectPath(2));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 4);
}

void PipelineManagerFunctionalTest::forkConnectionOrigin()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    TailFilterMockup *tail2 = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    Frame *frame;
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    CPPUNIT_ASSERT(pipe->addFilter(4, tail));
    CPPUNIT_ASSERT(pipe->addFilter(3, tail2));
    
    std::vector<int> midFilters({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 4, 1, -1, midFilters));
    
    std::vector<int> midFilters2;
    CPPUNIT_ASSERT(pipe->createPath(2, 1, 3, 1, -1, midFilters2));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->connectPath(2));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 1);
    CPPUNIT_ASSERT(tail2->getFrames() == 1);
    
    frame = tail->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 0);
    
    frame = tail2->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 0);
    
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
    CPPUNIT_ASSERT(tail2->getFrames() == 2);
    
    frame = tail->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 1);
    
    frame = tail2->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 1);
}

void PipelineManagerFunctionalTest::forkConnectionEnding()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    TailFilterMockup *tail2 = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    Frame *frame;
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    CPPUNIT_ASSERT(pipe->addFilter(4, tail));
    CPPUNIT_ASSERT(pipe->addFilter(3, tail2));
    
    std::vector<int> midFilters({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 3, 1, -1, midFilters));
    
    std::vector<int> midFilters2;
    CPPUNIT_ASSERT(pipe->createPath(2, 1, 4, 1, -1, midFilters2));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->connectPath(2));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 1);
    CPPUNIT_ASSERT(tail2->getFrames() == 1);
    
    frame = tail->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 0);
    
    frame = tail2->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 0);
    
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
    CPPUNIT_ASSERT(tail2->getFrames() == 2);
    
    frame = tail->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 1);
    
    frame = tail2->extract();
    CPPUNIT_ASSERT(frame && frame->getSequenceNumber() == 1);
}

void PipelineManagerFunctionalTest::forkedDiamondConnectionOrigin()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    TailFilterMockup *tail2 = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    OneToOneFilter *mid2 = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(4, tail));
    CPPUNIT_ASSERT(pipe->addFilter(5, tail2));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    CPPUNIT_ASSERT(pipe->addFilter(3, mid2));  
    
    std::vector<int> midFilters({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 4, 1, -1, midFilters));
    
    std::vector<int> midFilters2({3});
    CPPUNIT_ASSERT(pipe->createPath(2, 1, 4, 2, -1, midFilters2));
    
    std::vector<int> midFilters3;
    CPPUNIT_ASSERT(pipe->createPath(3, 1, 5, 3, -1, midFilters3));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->connectPath(2));
    CPPUNIT_ASSERT(pipe->connectPath(3));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
    CPPUNIT_ASSERT(tail2->getFrames() == 1);
    
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 4);
    CPPUNIT_ASSERT(tail2->getFrames() == 2);
}

void PipelineManagerFunctionalTest::forkedDiamondConnectionEnding()
{
    HeadFilterMockup *head = new HeadFilterMockup();
    TailFilterMockup *tail = new TailFilterMockup();
    TailFilterMockup *tail2 = new TailFilterMockup();
    OneToOneFilter *mid = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    OneToOneFilter *mid2 = new OneToOneFilterMockup(4, true, std::chrono::microseconds(0));
    
    CPPUNIT_ASSERT(pipe->addFilter(1, head));
    CPPUNIT_ASSERT(pipe->addFilter(4, tail));
    CPPUNIT_ASSERT(pipe->addFilter(5, tail2));
    CPPUNIT_ASSERT(pipe->addFilter(2, mid));
    CPPUNIT_ASSERT(pipe->addFilter(3, mid2));  
    
    std::vector<int> midFilters({2});
    CPPUNIT_ASSERT(pipe->createPath(1, 1, 4, 1, -1, midFilters));
    
    std::vector<int> midFilters2({3});
    CPPUNIT_ASSERT(pipe->createPath(2, 1, 4, 2, -1, midFilters2));
    
    std::vector<int> midFilters3;
    CPPUNIT_ASSERT(pipe->createPath(3, 3, 5, -1, -1, midFilters3));
    
    CPPUNIT_ASSERT(pipe->connectPath(1));
    CPPUNIT_ASSERT(pipe->connectPath(2));
    CPPUNIT_ASSERT(pipe->connectPath(3));
    
    CPPUNIT_ASSERT(head->inject(FrameMock::createNew(0)));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 2);
    CPPUNIT_ASSERT(tail2->getFrames() == 1);
    
    head->inject(FrameMock::createNew(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT));
    CPPUNIT_ASSERT(tail->getFrames() == 4);
    CPPUNIT_ASSERT(tail2->getFrames() == 2);
}

CPPUNIT_TEST_SUITE_REGISTRATION(PipelineManagerTest);
CPPUNIT_TEST_SUITE_REGISTRATION(PipelineManagerFunctionalTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("PipelineManagerTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();
    
    utils::printMood(runner.result().wasSuccessful());
    
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}
