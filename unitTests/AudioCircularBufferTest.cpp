/*
 *  AudioCircularBufferTest.cpp - AudioCircularBuffer class test
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

#include "AudioCircularBuffer.hh"
#include "Utils.hh"

typedef struct {
    unsigned char *data;
    unsigned size;
} Buffer;

class AudioCircularBufferTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AudioCircularBufferTest);
    CPPUNIT_TEST(create);
    CPPUNIT_TEST(normalBehaviour);
    CPPUNIT_TEST(timestampGap);
    CPPUNIT_TEST(timestampOverlapping);
    CPPUNIT_TEST(flushBecauseOfDeviation);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void create();
    void normalBehaviour();
    void timestampGap();
    void timestampOverlapping();
    void flushBecauseOfDeviation();

    AudioCircularBuffer* buffer;
    const unsigned channels = 2;
    const unsigned sampleRate = 48000;
    const unsigned maxSamples = 320;
    const SampleFmt format = S16P;
    const unsigned bytesPerSample = 2;
    std::chrono::milliseconds buffering = std::chrono::milliseconds(0);
};

void AudioCircularBufferTest::setUp()
{
    buffer = AudioCircularBuffer::createNew(channels, sampleRate, maxSamples, format, buffering);

    if (!buffer) {
        CPPUNIT_FAIL("AudioCircularBufferTest failed. Error creating AudioCircularBuffer in setUp()\n");
    }
}

void AudioCircularBufferTest::tearDown()
{
    delete buffer;
}

void AudioCircularBufferTest::create()
{
    unsigned badMaxSamples = 0;
    unsigned badChannels = 0;
    unsigned badSampleRate = 0;
    SampleFmt badFormat = FLT;
    AudioCircularBuffer* tmpbuffer;

    tmpbuffer = AudioCircularBuffer::createNew(badChannels, sampleRate, maxSamples, format, buffering);
    CPPUNIT_ASSERT(!tmpbuffer);

    tmpbuffer = AudioCircularBuffer::createNew(channels, badSampleRate, maxSamples, format, buffering);
    CPPUNIT_ASSERT(!tmpbuffer);
    
    tmpbuffer = AudioCircularBuffer::createNew(channels, sampleRate, badMaxSamples, format, buffering);
    CPPUNIT_ASSERT(!tmpbuffer);

    tmpbuffer = AudioCircularBuffer::createNew(channels, badSampleRate, maxSamples, badFormat, buffering);
    CPPUNIT_ASSERT(!tmpbuffer);

    tmpbuffer = AudioCircularBuffer::createNew(channels, sampleRate, maxSamples, format, buffering);
    CPPUNIT_ASSERT(tmpbuffer);

    delete tmpbuffer;
}

void AudioCircularBufferTest::normalBehaviour()
{
    Frame* inFrame;
    Frame* outFrame;
    AudioFrame* aFrame;
    bool newFrame;
    std::chrono::microseconds syncTime = std::chrono::microseconds(0);
    unsigned samplesPerFrame = 40;
    unsigned outputSamples = 80;
    unsigned insertedFrames = 0;
    unsigned removedFrames = 0;
    buffer->setOutputFrameSamples(outputSamples);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(2);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime);
    buffer->addFrame();
    insertedFrames++;
    
    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outFrame);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);  

    aFrame->fillWithValue(2);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds(insertedFrames*samplesPerFrame*std::micro::den/sampleRate));
    buffer->addFrame();
    insertedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == syncTime + std::chrono::microseconds(removedFrames*outputSamples*std::micro::den/sampleRate));
    removedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == syncTime + std::chrono::microseconds((removedFrames - 1)*outputSamples*std::micro::den/sampleRate));
    buffer->removeFrame();

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outFrame);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(2);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds(insertedFrames*samplesPerFrame*std::micro::den/sampleRate));
    buffer->addFrame();
    insertedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outFrame);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);  

    aFrame->fillWithValue(2);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds(insertedFrames*samplesPerFrame*std::micro::den/sampleRate));
    buffer->addFrame(); 

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == syncTime + std::chrono::microseconds(removedFrames*outputSamples*std::micro::den/sampleRate));
    removedFrames++;
}

void AudioCircularBufferTest::timestampGap()
{
    Frame* inFrame;
    Frame* outFrame;
    AudioFrame* aFrame;
    bool newFrame;
    std::chrono::microseconds syncTime = std::chrono::microseconds(0);
    const unsigned samplesPerFrame = 40;
    const unsigned outputSamples = 80;
    unsigned insertedFrames = 0;
    unsigned removedFrames = 0;
    const unsigned samplesGap = 80;
    const unsigned samplesValue = 2;
    buffer->setOutputFrameSamples(outputSamples);

    unsigned char valueTestBlock[samplesPerFrame*bytesPerSample];
    unsigned char zeroTestBlock[samplesPerFrame*bytesPerSample];
    memset(valueTestBlock, samplesValue, sizeof valueTestBlock);
    memset(zeroTestBlock, 0, sizeof zeroTestBlock);  

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime);
    buffer->addFrame();
    insertedFrames++;
    
    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);  

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds((samplesGap + insertedFrames*samplesPerFrame)*std::micro::den/sampleRate));
    buffer->addFrame();
    insertedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == syncTime + std::chrono::microseconds(removedFrames*outputSamples*std::micro::den/sampleRate));
    buffer->removeFrame();
    removedFrames++;

    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[0], valueTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[1], valueTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[0] + samplesPerFrame*bytesPerSample, zeroTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[1] + samplesPerFrame*bytesPerSample, zeroTestBlock, samplesPerFrame*bytesPerSample) == 0);

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == syncTime + std::chrono::microseconds(removedFrames*outputSamples*std::micro::den/sampleRate));
    removedFrames++;

    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[0], zeroTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[1], zeroTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[0] + samplesPerFrame*bytesPerSample, valueTestBlock, samplesPerFrame*bytesPerSample) == 0);
    CPPUNIT_ASSERT(memcmp(outFrame->getPlanarDataBuf()[1] + samplesPerFrame*bytesPerSample, valueTestBlock, samplesPerFrame*bytesPerSample) == 0);
}

void AudioCircularBufferTest::timestampOverlapping()
{
    Frame* inFrame;
    Frame* outFrame;
    AudioFrame* aFrame;
    bool newFrame;
    std::chrono::microseconds syncTime = std::chrono::microseconds(0);
    const unsigned samplesPerFrame = 100;
    const unsigned outputSamples = samplesPerFrame*2;
    unsigned insertedFrames = 0;
    const unsigned samplesValue = 2;
    buffer->setOutputFrameSamples(outputSamples);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime);
    buffer->addFrame();
    insertedFrames++;
    
    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);  

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds(samplesPerFrame/4*std::micro::den/sampleRate));
    buffer->addFrame();
    insertedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(!outFrame);
}

void AudioCircularBufferTest::flushBecauseOfDeviation()
{
    Frame* inFrame;
    Frame* outFrame;
    AudioFrame* aFrame;
    bool newFrame;
    std::chrono::microseconds syncTime = std::chrono::microseconds(0);
    std::chrono::microseconds newSyncTime = std::chrono::microseconds(1000);
    const unsigned samplesPerFrame = 100;
    const unsigned outputSamples = samplesPerFrame/2;
    unsigned insertedFrames = 0;
    const unsigned samplesValue = 2;
    buffer->setOutputFrameSamples(outputSamples);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime);
    buffer->addFrame();
    insertedFrames++;
    
    CPPUNIT_ASSERT(buffer->getElements() == insertedFrames*samplesPerFrame*bytesPerSample);

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(syncTime + std::chrono::microseconds((maxSamples + insertedFrames*samplesPerFrame)*std::micro::den/sampleRate));
    buffer->addFrame();
    insertedFrames++;

    CPPUNIT_ASSERT(buffer->getElements() == 0);
    insertedFrames = 0;

    inFrame = buffer->getRear();
    aFrame = dynamic_cast<AudioFrame*>(inFrame);   

    aFrame->fillWithValue(samplesValue);
    aFrame->setSamples(samplesPerFrame);
    aFrame->setPresentationTime(newSyncTime);
    buffer->addFrame();
    insertedFrames++;

    outFrame = buffer->getFront(newFrame);
    CPPUNIT_ASSERT(outFrame);
    CPPUNIT_ASSERT(outFrame->getPresentationTime() == newSyncTime);
    buffer->removeFrame();
}

CPPUNIT_TEST_SUITE_REGISTRATION(AudioCircularBufferTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("AudioCircularBufferTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}