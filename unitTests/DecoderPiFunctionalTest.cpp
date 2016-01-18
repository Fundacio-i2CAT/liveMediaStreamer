/*
 *  DecoderPiFunctionalTest.cpp - DecoderPi class test
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
#include "modules/decoderPi/DecoderPi.hh"

class DecoderPiFunctionalTest : public CppUnit::TestFixture {
	
	CPPUNIT_TEST_SUITE(DecoderPiFunctionalTest);
	CPPUNIT_TEST(decoderPiTest);
	CPPUNIT_TEST_SUITE_END();
	
	public:
		void setUp();
		void tearDown();
	
	protected:
		void decoderPiTest();

		OneToManyVideoScenarioMockup *decoderPiScenario;
		VideoSplitter* decoderPi;
		AVFramesReader* reader;
};

void DecoderPiFunctionalTest::setUp()
{
	decoderPi = DecoderPi::createNew();
	decoderPiScenario = new OneToManyVideoScenarioMockup(decoderPi, RAW, RGB24);
	reader = new AVFramesReader();
}

void DecoderPiFunctionalTest::tearDown()
{
	delete decoderPiScenario;
	delete decoderPi;
	delete reader;
}

void VideoSplitterFunctionalTest::splittingTest()
{
	InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame* decoderPiFrames;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoSplitterFunctionalTestInputImage.rgb", RAW, RGB24, 400, 400));
    frame = reader->getFrame();
    CPPUNIT_ASSERT(frame);
    decoderPiScenario->processFrame(frame);
    reader->close();

    
	std::string String = "testsData/videoSplitterFunctionalTestResImage" + std::to_string(f) + ".rgb";
	CPPUNIT_ASSERT(reader->openFile(String, RAW, RGB24, 100, 100));
	frame = reader->getFrame();
	CPPUNIT_ASSERT(frame);

	decoderPiFrames = splitterScenario->extractFrame(f);
	CPPUNIT_ASSERT(decoderPiFrames);
	
	CPPUNIT_ASSERT(frame->getLength() == decoderPiFrames->getLength());
	CPPUNIT_ASSERT(frame->getWidth() == decoderPiFrames->getWidth());
	CPPUNIT_ASSERT(frame->getHeight() == decoderPiFrames->getHeight());
	CPPUNIT_ASSERT(memcmp(frame->getDataBuf(), decoderPiFrames->getDataBuf(), frame->getLength()) == 0);
	reader->close();

}

CPPUNIT_TEST_SUITE_REGISTRATION(DecoderPiFunctionalTest);

int main(int argc, char* argv[])
{

	std::ofstream xmlout("DecoderPiFunctionalTest.xml");
	CPPUNIT_NS::TextTestRunner runner;
	CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
	runner.run("",false);
	outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}