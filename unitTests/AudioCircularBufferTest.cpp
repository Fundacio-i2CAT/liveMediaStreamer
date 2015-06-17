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
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void create();
    void okSliceBehaviour();
    void okCopySliceBehaviour();
    void okCopySlicesandSetSlicesBehaviour();
    void tooManySlices();

    AudioCircularBuffer* buffer;
    unsigned channels = 2;
    unsigned sampleRate = 48000;
    unsigned maxSamples = 4;
    SampleFmt format = S16P;
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