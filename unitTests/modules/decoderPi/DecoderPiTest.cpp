/*
 *  DecoderPiTest.cpp - DecoderPiTest class test
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
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "modules/decoderPi/DecoderPi.hh"

class DecoderPiMock : public DecoderPi {
	public:
	DecoderPiMock(std::chrono::microseconds fTime) : 
	DecoderPi(fTime){};
	using VideoSplitter::configCrop0;
	using VideoSplitter::specificWriterConfig;
	using VideoSplitter::specificWriterDelete;
};

class DecoderPiTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DecoderPiTest);
	CPPUNIT_TEST(constructorTest);
	CPPUNIT_TEST_SUITE_END();

	protected:
		void constructorTest();
};

void DecoderPiTest::constructorTest(){

	DecoderPi* decoderPi;

	std::chrono::microseconds negTime(-200);

	decoderPi = DecoderPi::createNew(negTime);
	CPPUNIT_ASSERT(!decoderPi);

	delete decoderPi;
}

CPPUNIT_TEST_SUITE_REGISTRATION(DecoderPiTest);

int main(int argc, char* argv[])
{
	std::ofstream xmlout("DecoderPiTest.xml");
	CPPUNIT_NS::TextTestRunner runner;
	CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
	runner.run("",false);
	outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}