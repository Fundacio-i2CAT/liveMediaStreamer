/*
 *  VideoSplitterFunctionalTest.cpp - VideoSplitter class test
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
 *  Authors:  Alejandro Jiménez <alejandro.jimenez@i2cat.net>
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
#include "modules/videoSplitter/VideoSplitter.hh"

class VideoSplitterFunctionalTest : public CppUnit::TestFixture {
	
	CPPUNIT_TEST_SUITE(VideoSplitterFunctionalTest);
	CPPUNIT_TEST(splittingTest);
	CPPUNIT_TEST_SUITE_END();
	
	public:
		void setUp();
		void tearDown();
	
	protected:
		void splittingTest();

		OneToManyVideoScenarioMockup *splitterScenario;
		VideoSplitter* splitter;
		AVFramesReader* reader;
};

void VideoSplitterFunctionalTest::setUp()
{
	splitter = VideoSplitter::createNew();
	splitterScenario = new OneToManyVideoScenarioMockup(splitter, RAW, RGB24);
	reader = new AVFramesReader();

	CPPUNIT_ASSERT(splitterScenario->addTailFilter(1));
	CPPUNIT_ASSERT(splitterScenario->addTailFilter(2));
	CPPUNIT_ASSERT(splitterScenario->addTailFilter(3));
	CPPUNIT_ASSERT(splitterScenario->addTailFilter(4));
	CPPUNIT_ASSERT(splitterScenario->addTailFilter(4));
	CPPUNIT_ASSERT(splitterScenario->connectFilters());
	CPPUNIT_ASSERT(splitter->configCrop(1,0.5,0.5,0,0));
	CPPUNIT_ASSERT(splitter->configCrop(2,0.5,0.5,0.5,0));
	CPPUNIT_ASSERT(splitter->configCrop(3,0.5,0.5,0,0.5));
	CPPUNIT_ASSERT(splitter->configCrop(4,0.5,0.5,0.5,0.5));
}

void VideoSplitterFunctionalTest::tearDown()
{
	delete splitterScenario;
	delete splitter;
	delete reader;
}

void VideoSplitterFunctionalTest::splittingTest()
{
	InterleavedVideoFrame *frame = NULL;
    std::map <int, InterleavedVideoFrame*> splitterFrames;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoSplitterFunctionalTestInputImage.rgb", RAW, RGB24, 400, 400));
    frame = reader->getFrame();
    CPPUNIT_ASSERT(frame);
    splitterScenario->processFrame(frame);
    reader->close();

    for(int f = 1; f <=4; f++){
    	std::string String = "testsData/videoSplitterFunctionalTestResImage" + std::to_string(f) + ".rgb";
    	CPPUNIT_ASSERT(reader->openFile(String, RAW, RGB24, 100, 100));
    	frame = reader->getFrame();
    	CPPUNIT_ASSERT(frame);

    	splitterFrames[f] = splitterScenario->extractFrame(f);
    	CPPUNIT_ASSERT(splitterFrames[f]);
    	
    	CPPUNIT_ASSERT(frame->getLength() == splitterFrames[f]->getLength());
    	CPPUNIT_ASSERT(frame->getWidth() == splitterFrames[f]->getWidth());
    	CPPUNIT_ASSERT(frame->getHeight() == splitterFrames[f]->getHeight());
    	CPPUNIT_ASSERT(memcmp(frame->getDataBuf(), splitterFrames[f]->getDataBuf(), frame->getLength()) == 0);
    	reader->close();
    }
    


}

CPPUNIT_TEST_SUITE_REGISTRATION(VideoSplitterFunctionalTest);

int main(int argc, char* argv[])
{

	std::ofstream xmlout("VideoSplitterFunctionalTest.xml");
	CPPUNIT_NS::TextTestRunner runner;
	CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
	runner.run("",false);
	outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}
