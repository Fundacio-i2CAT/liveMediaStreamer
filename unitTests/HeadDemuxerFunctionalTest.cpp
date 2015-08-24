/*
 *  HeadDemuxerFunctionalTest.cpp - Functional test for the HeadDemuxer filter
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
 *  Authors:  Xavi Artigas <xavier.artigas@i2cat.net>
 *
 */

#include <string>
#include <iostream>
#include <chrono>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "FilterFunctionalMockup.hh"
#include "modules/headDemuxer/HeadDemuxerLibav.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"
#include "modules/audioDecoder/AudioDecoderLibav.hh"

#define VIDEO_FRAMES    1000
#define BITS_X_BYTE     8

struct TestDataStruct {
    const char *uri;
    int width;
    int height;
    unsigned channels;
    unsigned sampleRate;
};

struct TestDataStruct TestData[] = {
    {   // 0
        "http://download.blender.org/durian/trailer/sintel_trailer-480p.mp4",
        854,
        480,
        2,
        48000
    },
    {   // 1
        "http://download.blender.org/durian/trailer/sintel_trailer-720p.mp4",
        1280,
        720,
        2,
        48000
    },
    {   // 2
        "http://samples.ffmpeg.org/V-codecs/h264/H.264+AAC(A-V%20sync)/H.264+AAC(A-V%20sync).mkv",
        704,
        480,
        2,
        48000
    },
    {   // 3
        "http://ftp.halifax.rwth-aachen.de/blender/demo/movies/ToS/tears_of_steel_720p.mov",
        1280,
        534,
        2,
        48000
    },
    {   // 4
        "http://h265.tvclever.com/TearsOfSteel_720p-h265.mkv",
        1280,
        720,
        2,
        48000
    }
};

class HeadDemuxerFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(HeadDemuxerFunctionalTest);
    CPPUNIT_TEST(test1);
    CPPUNIT_TEST(test2);
    CPPUNIT_TEST(test3);
    CPPUNIT_TEST(test4);
    CPPUNIT_TEST(test5);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void test(int test_ndx);
    void test1();
    void test2();
    void test3();
    void test4();
    void test5();

    HeadDemuxerLibav* headDemuxer;
    VideoDecoderLibav* vDecoder;
    AudioDecoderLibav* aDecoder;
    VideoTailFilterMockup* vTail;
    AudioTailFilterMockup* aTail;
};

void HeadDemuxerFunctionalTest::setUp()
{
    headDemuxer = new HeadDemuxerLibav();
    vDecoder = new VideoDecoderLibav();
    aDecoder = new AudioDecoderLibav();
    vTail = new VideoTailFilterMockup();
    aTail = new AudioTailFilterMockup();
}

void HeadDemuxerFunctionalTest::tearDown()
{
    headDemuxer->disconnectAll();
    vDecoder->disconnectAll();
    aDecoder->disconnectAll();
    vTail->disconnectAll();
    aTail->disconnectAll();
    delete headDemuxer;
    delete vDecoder;
    delete aDecoder;
    delete vTail;
    delete aTail;
}

void HeadDemuxerFunctionalTest::test1() {
    test(0);
}

void HeadDemuxerFunctionalTest::test2() {
    test(1);
}

void HeadDemuxerFunctionalTest::test3() {
    test(2);
}

void HeadDemuxerFunctionalTest::test4() {
    test(3);
}

void HeadDemuxerFunctionalTest::test5() {
    test(4);
}

void HeadDemuxerFunctionalTest::test(int test_ndx)
{
    Jzon::Object state;
    TestDataStruct *td = &TestData[test_ndx];

    CPPUNIT_ASSERT(headDemuxer->setURI(td->uri));
    headDemuxer->getState(state);
    CPPUNIT_ASSERT (state.Has("streams"));
    CPPUNIT_ASSERT (state.Get("streams").IsArray());
    const Jzon::Array &array = state.Get("streams").AsArray();

    // Chose an audio and a video stream at random (the last ones)
    int vWId, aWId;
    for (Jzon::Array::const_iterator it = array.begin(); it != array.end(); ++it) {
        int type = (*it).Get("type").ToInt();
        int wId = (*it).Get("wId").ToInt();
        switch (type) {
        case VIDEO:
            vWId = wId;
            break;
        case AUDIO:
            aWId = wId;
            break;
        default:
            break;
        }

    }

    // Build a simple pipeline: A Demuxer (which includes a source), with two
    // decoders and associated mockup tail filters to recover decoded frames.
    CPPUNIT_ASSERT(headDemuxer->connectManyToOne(vDecoder, vWId));
    CPPUNIT_ASSERT(headDemuxer->connectManyToOne(aDecoder, aWId));
    CPPUNIT_ASSERT(vDecoder->connectOneToOne(vTail));
    CPPUNIT_ASSERT(aDecoder->connectOneToOne(aTail));

    bool gotVideo = false, gotAudio = false;
    int watchDog = 0;
    int ret = 0;
    while (!gotVideo || !gotAudio) {
        headDemuxer->processFrame(ret);
        vDecoder->processFrame(ret);
        aDecoder->processFrame(ret);        
        vTail->processFrame(ret);
        aTail->processFrame(ret);
        InterleavedVideoFrame *vFrame = vTail->extract();
        PlanarAudioFrame *aFrame = aTail->extract();
        if (vFrame) {
            CPPUNIT_ASSERT (vFrame->getWidth() == td->width);
            CPPUNIT_ASSERT (vFrame->getHeight() == td->height);
            gotVideo = true;
        }
        if (aFrame) {
            CPPUNIT_ASSERT (aFrame->getChannels() == td->channels);
            CPPUNIT_ASSERT (aFrame->getSampleRate() == td->sampleRate);
            gotAudio = true;
        }

        watchDog++;
        if (watchDog > 25) {
            CPPUNIT_FAIL("Maximum number of iterations exceeded");
            // We should be able to retrieve that information in less than 25 iterations
            return;
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(HeadDemuxerFunctionalTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("HeadDemuxerFunctionalTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}
