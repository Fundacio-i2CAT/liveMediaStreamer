/*
 *  AudioMixerFunctionalTest.cpp - VideoMixingFunctionalTest class test
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
#include <chrono>
#include <fstream>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#include "FilterFunctionalMockup.hh"
#include "modules/audioMixer/AudioMixer.hh"

size_t readFile(char const* fileName, char* dstBuffer, size_t dstBuffLength)
{
    size_t inputDataSize;
    std::ifstream inputDataFile(fileName, std::ios::in|std::ios::binary|std::ios::ate);

    if (!inputDataFile.is_open()) {
        utils::errorMsg("Error opening file " + std::string(fileName));
        CPPUNIT_FAIL("Test data upload failed. Check test data file paths\n");
        return 0;
    }

    inputDataSize = inputDataFile.tellg();

    if (inputDataSize > dstBuffLength) {
        utils::errorMsg("Error opening file " + std::string(fileName) + ". Destionation buffer is not enough large.");
        CPPUNIT_FAIL("Test data upload failed. Check test data file paths\n");
    }

    inputDataFile.seekg (0, std::ios::beg);
    inputDataFile.read(dstBuffer, inputDataSize);
    return inputDataSize;
}

class AudioMixerFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AudioMixerFunctionalTest);
    CPPUNIT_TEST(mixingTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void mixingTest();

    int channels = 2;
    int sampleRate = 48000;
    SampleFmt sFmt = FLTP;

    int mixWidth = 1920;
    int mixHeight = 1080;

    ManyToOneAudioScenarioMockup *mixScenario;
    AudioMixer* mixer;
    PlanarAudioFrame* inputFrame;
};

void AudioMixerFunctionalTest::setUp()
{
    float fValue;
    int nOfSamples;
    int bytesPerSample;
    unsigned char* chBuff;

    fValue = 0.5;
    bytesPerSample = utils::getBytesPerSampleFromFormat(sFmt);

    mixer = new AudioMixer();
    mixScenario = new ManyToOneAudioScenarioMockup(mixer);

    nOfSamples = mixer->getInputFrameSamples();

    CPPUNIT_ASSERT(mixScenario->addHeadFilter(1, channels, sampleRate, sFmt)); 
    CPPUNIT_ASSERT(mixScenario->addHeadFilter(2, channels, sampleRate, sFmt)); 
    CPPUNIT_ASSERT(mixScenario->connectFilters());

    inputFrame = PlanarAudioFrame::createNew(channels, sampleRate, AudioFrame::getMaxSamples(sampleRate), PCM, sFmt);

    for (int c = 0; c < channels; c++) {
        chBuff = inputFrame->getPlanarDataBuf()[c];

        for (int i = 0; i < nOfSamples; i++) {
            AudioMixer::floatToBytes(chBuff + i*bytesPerSample, fValue, sFmt);
        }
    }

    inputFrame->setLength(nOfSamples*bytesPerSample);
    inputFrame->setSamples(nOfSamples);
}

void AudioMixerFunctionalTest::tearDown()
{
    delete mixScenario;
    delete mixer;
}

void AudioMixerFunctionalTest::mixingTest()
{
    PlanarAudioFrame* mixedFrame;
    std::chrono::microseconds originTs;
    std::chrono::microseconds tsIncrement;
    std::chrono::microseconds ts;
    unsigned introducedSamples;
    unsigned mixThresholdSamples;
    std::string modelFileName;
    std::ofstream fout;
    char* modelBuff;
    size_t modelLength;

    introducedSamples = 0;
    mixThresholdSamples = mixer->getMixingThreshold();
    originTs = std::chrono::microseconds(40000);
    tsIncrement = std::chrono::microseconds(inputFrame->getSamples()*std::micro::den/sampleRate);
    ts = originTs;

    while(1) {

        inputFrame->setPresentationTime(ts);
        mixScenario->processFrame(inputFrame);
        introducedSamples += inputFrame->getSamples();

        if (introducedSamples >= mixThresholdSamples) {
            break;
        }

        mixedFrame = mixScenario->extractFrame();
        CPPUNIT_ASSERT(!mixedFrame);
        ts += tsIncrement;
    }

    mixedFrame = mixScenario->extractFrame();
    CPPUNIT_ASSERT(mixedFrame);
    CPPUNIT_ASSERT(mixedFrame->getPresentationTime() == originTs);
    CPPUNIT_ASSERT(mixedFrame->getSamples() == mixer->getInputFrameSamples());
    CPPUNIT_ASSERT(mixedFrame->getLength() == mixer->getInputFrameSamples()*utils::getBytesPerSampleFromFormat(mixedFrame->getSampleFmt()));

    modelBuff = new char[mixedFrame->getLength()]();

    for (unsigned i = 0; i < mixedFrame->getChannels(); i++) {
        modelFileName = "testsData/audioMixerFunctionalTestModel_channel_" + std::to_string(i);
        modelLength = readFile(modelFileName.c_str(), modelBuff, mixedFrame->getLength());
        CPPUNIT_ASSERT(mixedFrame->getLength() == modelLength);
        CPPUNIT_ASSERT(memcmp(modelBuff, mixedFrame->getPlanarDataBuf()[i], modelLength) == 0);
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(AudioMixerFunctionalTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("AudioMixerFunctionalTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());
    delete outputter;

    return runner.result().wasSuccessful() ? 0 : 1;
}