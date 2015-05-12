/*
 *  X265VideoCircularBufferTest.cpp - X265VideoCircularBuffer class test
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

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "X265VideoCircularBuffer.hh"
#include "Utils.hh"

class X265VideoCircularBufferTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(X265VideoCircularBufferTest);
    CPPUNIT_TEST(createNew);
    CPPUNIT_TEST(okNalBehaviour);
    CPPUNIT_TEST(okHdrNalBehaviour);
    CPPUNIT_TEST(okHdrNalAndNalBehaviour);
    CPPUNIT_TEST(tooManyNals);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void createNew();
    void okNalBehaviour();
    void okHdrNalBehaviour();
    void okHdrNalAndNalBehaviour();
    void tooManyNals();

    unsigned maxFrames = 4;
    X265VideoCircularBuffer* buffer;
};

void X265VideoCircularBufferTest::setUp()
{
    buffer = X265VideoCircularBuffer::createNew(maxFrames);

    if (!buffer) {
        CPPUNIT_FAIL("X265VideoCircularBufferTest failed. Error creating X265VideoCircularBuffer in setUp()\n");
    }
}

void X265VideoCircularBufferTest::tearDown()
{
    delete buffer;
}

void X265VideoCircularBufferTest::createNew()
{
    int tmpMaxFrames;
    X265VideoCircularBuffer* tmpBuffer;

    tmpMaxFrames = 0;
    tmpBuffer = X265VideoCircularBuffer::createNew(tmpMaxFrames);
    CPPUNIT_ASSERT(!tmpBuffer);

    tmpMaxFrames = 1;
    tmpBuffer = X265VideoCircularBuffer::createNew(tmpMaxFrames);
    CPPUNIT_ASSERT(tmpBuffer);

    delete tmpBuffer;
}

void X265VideoCircularBufferTest::okNalBehaviour()
{
    bool newFrame = false;
    unsigned nalNumber = 2;
    int nalSize = 1;
    x265_nal nals[nalNumber];
    X265VideoFrame* x265Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].payload = new unsigned char [nalSize];
        nals[i].sizeBytes = nalSize;
        std::fill_n(nals[i].payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x265Frame = dynamic_cast<X265VideoFrame*>(inputFrame);
    x265Frame->setNals(nals);
    x265Frame->setNalNum(nalNumber);
    buffer->addFrame();
    CPPUNIT_ASSERT(buffer->getElements() == nalNumber);

    for (unsigned i = 0; i < nalNumber; i++) {
        outputFrame = buffer->getFront(newFrame);
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(newFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        buffer->removeFrame();
    }

    CPPUNIT_ASSERT(buffer->getElements() == 0);
    outputFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outputFrame);
    CPPUNIT_ASSERT(!newFrame);
}

void X265VideoCircularBufferTest::okHdrNalBehaviour()
{
    bool newFrame = false;
    unsigned nalNumber = 2;
    int nalSize = 1;
    x265_nal nals[nalNumber];
    X265VideoFrame* x265Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].payload = new unsigned char [nalSize];
        nals[i].sizeBytes = nalSize;
        std::fill_n(nals[i].payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x265Frame = dynamic_cast<X265VideoFrame*>(inputFrame);
    x265Frame->setHdrNals(nals);
    x265Frame->setHdrNalNum(nalNumber);
    buffer->addFrame();
    CPPUNIT_ASSERT(buffer->getElements() == nalNumber);

    for (unsigned i = 0; i < nalNumber; i++) {
        outputFrame = buffer->getFront(newFrame);
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(newFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        buffer->removeFrame();
    }

    CPPUNIT_ASSERT(buffer->getElements() == 0);
    outputFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outputFrame);
    CPPUNIT_ASSERT(!newFrame);
}

void X265VideoCircularBufferTest::okHdrNalAndNalBehaviour()
{
    bool newFrame = false;
    int nalSize = 1;
    
    unsigned nalNumber = 2;
    unsigned hdrNalNumber = 1;
    
    x265_nal nals[nalNumber];
    x265_nal hdrNals[hdrNalNumber];

    X265VideoFrame* x265Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].payload = new unsigned char [nalSize];
        nals[i].sizeBytes = nalSize;
        std::fill_n(nals[i].payload, nalSize, i);
    }

    for (unsigned i = 0; i < hdrNalNumber; i++) {
        hdrNals[i].payload = new unsigned char [nalSize];
        hdrNals[i].sizeBytes = nalSize;
        std::fill_n(hdrNals[i].payload, nalSize, i + nalNumber);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x265Frame = dynamic_cast<X265VideoFrame*>(inputFrame);
    x265Frame->setNals(nals);
    x265Frame->setNalNum(nalNumber);
    x265Frame->setHdrNals(hdrNals);
    x265Frame->setHdrNalNum(hdrNalNumber);
    buffer->addFrame();

    CPPUNIT_ASSERT(buffer->getElements() == nalNumber + hdrNalNumber);
    
    for (unsigned i = 0; i < hdrNalNumber; i++) {
        outputFrame = buffer->getFront(newFrame);
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(newFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i + nalNumber);
        buffer->removeFrame();
    }

    for (unsigned i = 0; i < nalNumber; i++) {
        outputFrame = buffer->getFront(newFrame);
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(newFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        buffer->removeFrame();
    }

    CPPUNIT_ASSERT(buffer->getElements() == 0);
    outputFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outputFrame);
    CPPUNIT_ASSERT(!newFrame);
}

void X265VideoCircularBufferTest::tooManyNals()
{
    bool newFrame = false;
    unsigned nalNumber = 10;
    int nalSize = 1;
    x265_nal nals[nalNumber];
    X265VideoFrame* x265Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    CPPUNIT_ASSERT(nalNumber > maxFrames);

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].payload = new unsigned char [nalSize];
        nals[i].sizeBytes = nalSize;
        std::fill_n(nals[i].payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x265Frame = dynamic_cast<X265VideoFrame*>(inputFrame);
    x265Frame->setNals(nals);
    x265Frame->setNalNum(nalNumber);
    buffer->addFrame();
    CPPUNIT_ASSERT(buffer->getElements() == maxFrames);

    inputFrame = buffer->getRear();
    CPPUNIT_ASSERT(!inputFrame);

    for (unsigned i = 0; i < maxFrames - 1; i++) {
        outputFrame = buffer->getFront(newFrame);
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(newFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        buffer->removeFrame();
    }

    outputFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outputFrame);
    CPPUNIT_ASSERT(newFrame);
    CPPUNIT_ASSERT(*outputFrame->getDataBuf() == nalNumber - 1);
    buffer->removeFrame();

    CPPUNIT_ASSERT(buffer->getElements() == 0);
    outputFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outputFrame);
    CPPUNIT_ASSERT(!newFrame);
}

CPPUNIT_TEST_SUITE_REGISTRATION(X265VideoCircularBufferTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("X265VideoCircularBufferTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}