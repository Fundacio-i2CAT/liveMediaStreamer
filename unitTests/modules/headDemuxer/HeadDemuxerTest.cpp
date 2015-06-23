/*
 *  HeadDemuxerTest.cpp - HeadDemuxerLibav class test
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
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "modules/headDemuxer/HeadDemuxerLibav.hh"

class HeadDemuxerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(HeadDemuxerTest);
    CPPUNIT_TEST(demuxingTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void demuxingTest();

    HeadDemuxerLibav* demuxer;
};

void HeadDemuxerTest::setUp()
{
    demuxer = new HeadDemuxerLibav();
}

void HeadDemuxerTest::tearDown()
{
    delete demuxer;
}

void HeadDemuxerTest::demuxingTest()
{
    static const std::string bad_uri1 = "test://non.existing.protocol";
    static const std::string bad_uri2 = "http://non.existing.host";
    static const std::string bad_uri3 = "http://docs.gstreamer.com/display/GstSDK/gst-inspect";
    static const std::string good_uri = "http://docs.gstreamer.com/media/sintel_trailer-480p.webm";
    Jzon::Object state;

    CPPUNIT_ASSERT (demuxer->setURI (bad_uri1) == false);
    CPPUNIT_ASSERT (demuxer->setURI (bad_uri2) == false);
    CPPUNIT_ASSERT (demuxer->setURI (bad_uri3) == false);
    CPPUNIT_ASSERT (demuxer->setURI (good_uri));
    demuxer->getState (state);
    Jzon::Node &node = state.Get("uri");
    CPPUNIT_ASSERT (node.IsValue());
    CPPUNIT_ASSERT (node.AsValue().GetValueType() == Jzon::Value::VT_STRING);
    CPPUNIT_ASSERT (node.ToString() == good_uri);

    Jzon::Array &array = state.Get("streams").AsArray();
    CPPUNIT_ASSERT (array.GetCount() == 2);

    Jzon::Node &vnode = array.Get(0);
    CPPUNIT_ASSERT (vnode.Get("wId").ToInt() == 0);
    CPPUNIT_ASSERT (vnode.Get("type").ToInt() == VIDEO);
    CPPUNIT_ASSERT (vnode.Get("codec_name").ToString() == "vp8");

    Jzon::Node &anode = array.Get(1);
    CPPUNIT_ASSERT (anode.Get("wId").ToInt() == 1);
    CPPUNIT_ASSERT (anode.Get("type").ToInt() == AUDIO);
    CPPUNIT_ASSERT (anode.Get("codec_name").ToString() == "vorbis");

    /* // Uncomment to dump state Json
    Jzon::Writer writer(state, Jzon::StandardFormat);
    writer.Write();
    std::cerr << writer.GetResult();
    */
}

CPPUNIT_TEST_SUITE_REGISTRATION(HeadDemuxerTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("HeadDemuxerTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    delete outputter;

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
