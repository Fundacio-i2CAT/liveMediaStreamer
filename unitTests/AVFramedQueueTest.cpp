/*
 *  AVFramedQueueTest.cpp - AudioCircularBuffer class test
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *
 */

#include <string>
#include <iostream>
#include <fstream>
#include <string.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "AVFramedQueue.hh"
#include "FilterMockup.hh"
#include "Utils.hh"
#include "StreamInfo.hh"

class AVFramedQueueTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AVFramedQueueTest);
    CPPUNIT_TEST(normalBehaviour);
    CPPUNIT_TEST(forceGetRearTest);
    CPPUNIT_TEST(forceGetFrontTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void normalBehaviour();
    void forceGetRearTest();
    void forceGetFrontTest();

    struct ConnectionData cData;
    unsigned maxFrames;

    AVFramedQueue* q;
};

void AVFramedQueueTest::setUp()
{
    maxFrames = 4;
    q = new AVFramedQueueMock(cData, &mockStreamInfo, maxFrames);
}

void AVFramedQueueTest::tearDown()
{
    delete q;
}

void AVFramedQueueTest::normalBehaviour()
{
    Frame* frame = NULL;
    size_t seq = 0;

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        frame = q->getRear();
        CPPUNIT_ASSERT(frame);
        frame->setSequenceNumber(seq++);
        CPPUNIT_ASSERT(q->getElements() == i);
        CPPUNIT_ASSERT(q->addFrame() == cData.rFilterId);
        CPPUNIT_ASSERT(q->getElements() == i + 1);
    }

    frame = q->getRear();
    CPPUNIT_ASSERT(!frame);
    seq = 0;

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        frame = q->getFront();
        CPPUNIT_ASSERT(frame);
        CPPUNIT_ASSERT(frame->getSequenceNumber() == seq++);
        CPPUNIT_ASSERT(q->removeFrame() == cData.wFilterId);
    }

    frame = q->getFront();
    CPPUNIT_ASSERT(!frame);
}

void AVFramedQueueTest::forceGetRearTest()
{
    Frame* frame = NULL;
    size_t seq = 0;

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        frame = q->getRear();
        CPPUNIT_ASSERT(frame);
        frame->setSequenceNumber(seq++);
        CPPUNIT_ASSERT(q->getElements() == i);
        CPPUNIT_ASSERT(q->addFrame() == cData.rFilterId);
        CPPUNIT_ASSERT(q->getElements() == i + 1);
    }

    frame = q->getRear();
    CPPUNIT_ASSERT(!frame);

    frame = q->forceGetRear();
    CPPUNIT_ASSERT(frame);
    frame->setSequenceNumber(seq++);
    CPPUNIT_ASSERT(q->getElements() == maxFrames - 2);
    CPPUNIT_ASSERT(q->addFrame() == cData.rFilterId);
    CPPUNIT_ASSERT(q->getElements() == maxFrames - 1);

    seq = 0;

    for (unsigned i = 0; i < maxFrames - 2; i++) {
        frame = q->getFront();
        CPPUNIT_ASSERT(frame);
        CPPUNIT_ASSERT(frame->getSequenceNumber() == seq++);
        CPPUNIT_ASSERT(q->removeFrame() == cData.wFilterId);
    }

    frame = q->getFront();
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(frame->getSequenceNumber() == seq + 1);
    CPPUNIT_ASSERT(q->removeFrame() == cData.wFilterId);

    frame = q->getFront();
    CPPUNIT_ASSERT(!frame);
}

void AVFramedQueueTest::forceGetFrontTest()
{
    Frame* frame = NULL;
    size_t seq = 0;

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        frame = q->getRear();
        CPPUNIT_ASSERT(frame);
        frame->setSequenceNumber(seq++);
        CPPUNIT_ASSERT(q->getElements() == i);
        CPPUNIT_ASSERT(q->addFrame() == cData.rFilterId);
        CPPUNIT_ASSERT(q->getElements() == i + 1);
    }

    frame = q->getRear();
    CPPUNIT_ASSERT(!frame);
    seq = 0;

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        frame = q->getFront();
        CPPUNIT_ASSERT(frame);
        CPPUNIT_ASSERT(frame->getSequenceNumber() == seq++);
        CPPUNIT_ASSERT(q->removeFrame() == cData.wFilterId);
    }

    frame = q->getFront();
    CPPUNIT_ASSERT(!frame);

    frame = q->forceGetFront();
    CPPUNIT_ASSERT(frame);
    CPPUNIT_ASSERT(frame->getSequenceNumber() == seq - 1);
}

CPPUNIT_TEST_SUITE_REGISTRATION(AVFramedQueueTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("AVFramedQueueTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
