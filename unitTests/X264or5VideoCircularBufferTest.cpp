/*
 *  X264or5VideoCircularBufferTest.cpp - X264or5VideoCircularBufferTest class test
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

#include "X264or5VideoCircularBuffer.hh"
#include "FilterMockup.hh"

class x264or5VideoCircularBufferMock : public X264or5VideoCircularBuffer {

public:
    x264or5VideoCircularBufferMock(VCodecType codec, unsigned maxFrames)
    :  X264or5VideoCircularBuffer(codec, maxFrames)
    {
        inputFrame = new FrameMock();
    }

    ~x264or5VideoCircularBufferMock()
    {
        delete inputFrame;
    }

    using X264or5VideoCircularBuffer::setup;
    Frame* getInputFrame(){return inputFrame;};

    bool pushBack()
    {
        Frame* frame;

        frame = innerGetRear();

        if (!frame && !forceGetRear) {
            return false;
        }

        if (!frame && forceGetRear) {
            frame = innerForceGetRear();
        }

        if (!frame) {
            return false;
        }

        innerAddFrame();
        return true;
    }

    void setForceGetRear(bool f){forceGetRear = f;};
    
private:
    FrameMock* inputFrame;
    bool forceGetRear;
};

class x264or5VideoCircularBufferTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(x264or5VideoCircularBufferTest);
    CPPUNIT_TEST(setupTest);
    CPPUNIT_TEST(pushBackNoForce);
    CPPUNIT_TEST(pushBackForce);
    CPPUNIT_TEST_SUITE_END();

protected:
    void setupTest();
    void pushBackNoForce();
    void pushBackForce();

    x264or5VideoCircularBufferMock* buffer;
};

void x264or5VideoCircularBufferTest::setupTest()
{
    unsigned maxFrames = 4;
    
    buffer = new x264or5VideoCircularBufferMock(VP8, maxFrames);
    CPPUNIT_ASSERT(!buffer->setup());
    delete buffer;

    buffer = new x264or5VideoCircularBufferMock(H264, maxFrames);
    CPPUNIT_ASSERT(buffer->setup());
    delete buffer;

    buffer = new x264or5VideoCircularBufferMock(H265, maxFrames);
    CPPUNIT_ASSERT(buffer->setup());
    delete buffer;
}

void x264or5VideoCircularBufferTest::pushBackNoForce()
{
    unsigned maxFrames = 4;
    buffer = new x264or5VideoCircularBufferMock(H264, maxFrames);
    buffer->setup();

    buffer->setForceGetRear(false);

    for (unsigned i = 0; i < maxFrames; i++) {
        CPPUNIT_ASSERT(buffer->pushBack());
    }

    CPPUNIT_ASSERT(!buffer->pushBack());
    delete buffer;
}

void x264or5VideoCircularBufferTest::pushBackForce()
{
    unsigned maxFrames = 4;
    buffer = new x264or5VideoCircularBufferMock(H264, maxFrames);
    buffer->setup();

    buffer->setForceGetRear(true);

    for (unsigned i = 0; i < maxFrames; i++) {
        CPPUNIT_ASSERT(buffer->pushBack());
    }

    CPPUNIT_ASSERT(buffer->pushBack());
}

CPPUNIT_TEST_SUITE_REGISTRATION(x264or5VideoCircularBufferTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("x264or5VideoCircularBufferTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}