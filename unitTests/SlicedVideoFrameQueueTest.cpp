/*
 *  SlicedVideoFrameQueueTest.cpp - SlicedVideoFrameQueue class test
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

#include "SlicedVideoFrameQueue.hh"
#include "Utils.hh"
#include "StreamInfo.hh"

typedef struct {
    unsigned char *data;
    unsigned size;
} Buffer;

static StreamInfo mockStreamInfo = { VIDEO };

class SlicedVideoFrameQueueTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(SlicedVideoFrameQueueTest);
    CPPUNIT_TEST(create);
    CPPUNIT_TEST(okSliceBehaviour);
    CPPUNIT_TEST(tooManySlices);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void create();
    void okSliceBehaviour();
    void tooManySlices();

    SlicedVideoFrameQueue* queue;
    unsigned maxFrames;
    unsigned maxSliceSize;
    struct ConnectionData cData;
};

void SlicedVideoFrameQueueTest::setUp()
{
    mockStreamInfo.video.codec = H264;
    maxFrames = 4;
    maxSliceSize = 16;
    queue = SlicedVideoFrameQueue::createNew(cData, &mockStreamInfo, maxFrames, maxSliceSize);

    if (!queue) {
        CPPUNIT_FAIL("SlicedVideoFrameQueueTest failed. Error creating SlicedVideoFrameQueue in setUp()\n");
    }
}

void SlicedVideoFrameQueueTest::tearDown()
{
    delete queue;
}

void SlicedVideoFrameQueueTest::create()
{
    int tmpMaxFrames;
    int tmpMaxSliceSize;
    SlicedVideoFrameQueue* tmpqueue;

    tmpMaxFrames = 0;
    tmpMaxSliceSize = 0;
    tmpqueue = SlicedVideoFrameQueue::createNew(cData, &mockStreamInfo, tmpMaxFrames, tmpMaxSliceSize);
    CPPUNIT_ASSERT(!tmpqueue);

    tmpMaxFrames = 0;
    tmpMaxSliceSize = 16;
    tmpqueue = SlicedVideoFrameQueue::createNew(cData, &mockStreamInfo, tmpMaxFrames, tmpMaxSliceSize);
    CPPUNIT_ASSERT(!tmpqueue);

    tmpMaxFrames = 16;
    tmpMaxSliceSize = 0;
    tmpqueue = SlicedVideoFrameQueue::createNew(cData, &mockStreamInfo, tmpMaxFrames, tmpMaxSliceSize);
    CPPUNIT_ASSERT(!tmpqueue);

    tmpMaxFrames = 16;
    tmpMaxSliceSize = 16;
    tmpqueue = SlicedVideoFrameQueue::createNew(cData, &mockStreamInfo, tmpMaxFrames, tmpMaxSliceSize);
    CPPUNIT_ASSERT(tmpqueue);
    
    CPPUNIT_ASSERT(tmpqueue->getRear());

    delete tmpqueue;
}

void SlicedVideoFrameQueueTest::okSliceBehaviour()
{
    unsigned buffersNum = 2;
    int bufferDataSize = 1;
    Buffer buffers[buffersNum];
    SlicedVideoFrame* slicedFrame;
    Frame* inputFrame;
    Frame* outputFrame;

    for (unsigned i = 0; i < buffersNum; i++) {
        buffers[i].data = new unsigned char [bufferDataSize];
        buffers[i].size = bufferDataSize;
        std::fill_n(buffers[i].data, bufferDataSize, i);
    }

    inputFrame = queue->getRear();

    CPPUNIT_ASSERT(inputFrame);
    slicedFrame = dynamic_cast<SlicedVideoFrame*>(inputFrame);

    for (unsigned i = 0; i < buffersNum; i++) {
        CPPUNIT_ASSERT(slicedFrame->setSlice(buffers[i].data, buffers[i].size));
    }

    queue->addFrame();
    CPPUNIT_ASSERT(queue->getElements() == buffersNum);

    for (unsigned i = 0; i < buffersNum; i++) {
        outputFrame = queue->getFront();
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        queue->removeFrame();
    }

    CPPUNIT_ASSERT(queue->getElements() == 0);
    outputFrame = queue->getFront();
    CPPUNIT_ASSERT(!outputFrame);
}


void SlicedVideoFrameQueueTest::tooManySlices()
{
    unsigned buffersNum = 10;
    int bufferDataSize = 1;
    Buffer buffers[buffersNum];
    SlicedVideoFrame* slicedFrame;
    Frame* inputFrame;
    Frame* outputFrame;

    CPPUNIT_ASSERT(buffersNum > maxFrames);

    for (unsigned i = 0; i < buffersNum; i++) {
        buffers[i].data = new unsigned char [bufferDataSize];
        buffers[i].size = bufferDataSize;
        std::fill_n(buffers[i].data, bufferDataSize, i);
    }

    inputFrame = queue->getRear();

    CPPUNIT_ASSERT(inputFrame);
    slicedFrame = dynamic_cast<SlicedVideoFrame*>(inputFrame);
    
    for (unsigned i = 0; i < buffersNum; i++) {
        CPPUNIT_ASSERT(slicedFrame->setSlice(buffers[i].data, buffers[i].size));
    }
    
    queue->addFrame();
    CPPUNIT_ASSERT(queue->getElements() == maxFrames - 1);

    inputFrame = queue->getRear();
    CPPUNIT_ASSERT(!inputFrame);

    for (unsigned i = 0; i < maxFrames - 2; i++) {
        outputFrame = queue->getFront();
        CPPUNIT_ASSERT(outputFrame);
        CPPUNIT_ASSERT(*outputFrame->getDataBuf() == i);
        queue->removeFrame();
    }

    outputFrame = queue->getFront();
    CPPUNIT_ASSERT(outputFrame);
    CPPUNIT_ASSERT(*outputFrame->getDataBuf() == buffersNum - 1);
    queue->removeFrame();

    CPPUNIT_ASSERT(queue->getElements() == 0);
    outputFrame = queue->getFront();
    CPPUNIT_ASSERT(!outputFrame);
}

CPPUNIT_TEST_SUITE_REGISTRATION(SlicedVideoFrameQueueTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("SlicedVideoFrameQueueTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
