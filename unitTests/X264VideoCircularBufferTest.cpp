/*
 *  X264VideoCircularBufferTest.cpp - X264VideoCircularBuffer class test
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

#include "X264VideoCircularBuffer.hh"
#include "Utils.hh"

class X264VideoCircularBufferTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(X264VideoCircularBufferTest);
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
    X264VideoCircularBuffer* buffer;
};

void X264VideoCircularBufferTest::setUp()
{
    buffer = X264VideoCircularBuffer::createNew(maxFrames);

    if (!buffer) {
        CPPUNIT_FAIL("X264VideoCircularBufferTest failed. Error creating X264VideoCircularBuffer in setUp()\n");
    }
}

void X264VideoCircularBufferTest::tearDown()
{
    delete buffer;
}

void X264VideoCircularBufferTest::createNew()
{
    int tmpMaxFrames;
    X264VideoCircularBuffer* tmpBuffer;

    tmpMaxFrames = 0;
    tmpBuffer = X264VideoCircularBuffer::createNew(tmpMaxFrames);
    CPPUNIT_ASSERT(!tmpBuffer);

    tmpMaxFrames = 1;
    tmpBuffer = X264VideoCircularBuffer::createNew(tmpMaxFrames);
    CPPUNIT_ASSERT(tmpBuffer);

    delete tmpBuffer;
}

void X264VideoCircularBufferTest::okNalBehaviour()
{
    bool newFrame = false;
    unsigned nalNumber = 2;
    int nalSize = 1;
    x264_nal_t nals[nalNumber];
    X264VideoFrame* x264Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].p_payload = new unsigned char [nalSize];
        nals[i].i_payload = nalSize;
        std::fill_n(nals[i].p_payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x264Frame = dynamic_cast<X264VideoFrame*>(inputFrame);
    x264Frame->setNals(nals);
    x264Frame->setNalNum(nalNumber);
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

void X264VideoCircularBufferTest::okHdrNalBehaviour()
{
    bool newFrame = false;
    unsigned nalNumber = 2;
    int nalSize = 1;
    x264_nal_t nals[nalNumber];
    X264VideoFrame* x264Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].p_payload = new unsigned char [nalSize];
        nals[i].i_payload = nalSize;
        std::fill_n(nals[i].p_payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x264Frame = dynamic_cast<X264VideoFrame*>(inputFrame);
    x264Frame->setHdrNals(nals);
    x264Frame->setHdrNalNum(nalNumber);
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

void X264VideoCircularBufferTest::okHdrNalAndNalBehaviour()
{
    bool newFrame = false;
    int nalSize = 1;
    
    unsigned nalNumber = 2;
    unsigned hdrNalNumber = 1;
    
    x264_nal_t nals[nalNumber];
    x264_nal_t hdrNals[hdrNalNumber];

    X264VideoFrame* x264Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].p_payload = new unsigned char [nalSize];
        nals[i].i_payload = nalSize;
        std::fill_n(nals[i].p_payload, nalSize, i);
    }

    for (unsigned i = 0; i < hdrNalNumber; i++) {
        hdrNals[i].p_payload = new unsigned char [nalSize];
        hdrNals[i].i_payload = nalSize;
        std::fill_n(hdrNals[i].p_payload, nalSize, i + nalNumber);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x264Frame = dynamic_cast<X264VideoFrame*>(inputFrame);
    x264Frame->setNals(nals);
    x264Frame->setNalNum(nalNumber);
    x264Frame->setHdrNals(hdrNals);
    x264Frame->setHdrNalNum(hdrNalNumber);
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

void X264VideoCircularBufferTest::tooManyNals()
{
    bool newFrame = false;
    unsigned nalNumber = 10;
    int nalSize = 1;
    x264_nal_t nals[nalNumber];
    X264VideoFrame* x264Frame;
    Frame* inputFrame;
    Frame* outputFrame;

    CPPUNIT_ASSERT(nalNumber > maxFrames);

    for (unsigned i = 0; i < nalNumber; i++) {
        nals[i].p_payload = new unsigned char [nalSize];
        nals[i].i_payload = nalSize;
        std::fill_n(nals[i].p_payload, nalSize, i);
    }

    inputFrame = buffer->getRear();

    CPPUNIT_ASSERT(inputFrame);
    x264Frame = dynamic_cast<X264VideoFrame*>(inputFrame);
    x264Frame->setNals(nals);
    x264Frame->setNalNum(nalNumber);
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

CPPUNIT_TEST_SUITE_REGISTRATION(X264VideoCircularBufferTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("X264VideoCircularBufferTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}