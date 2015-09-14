/*
 *  MpdManagerTest.cpp - MpdManager class test
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors:  Xavier Carol <xavier.carol@i2cat.net>
 *
 *
 */
#include <unistd.h>

#include "modules/dasher/MpdManager.hh"

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>

#define FILE_NAME "/tmp/mpdmanagertest.xml"
#define MAX_SEGMENTS 4

class MpdManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(MpdManagerTest);
    CPPUNIT_TEST(writeToDisk);
    CPPUNIT_TEST(configureTest);
    CPPUNIT_TEST(updateVideoAdaptationSet);
    CPPUNIT_TEST(updateAudioAdaptationSet);
    CPPUNIT_TEST(updateAdaptationSetTimestamp);
    CPPUNIT_TEST(updateVideoRepresentation);
    CPPUNIT_TEST(updateAudioRepresentation);
    CPPUNIT_TEST(removeRepresentation);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void writeToDisk();
    void configureTest();
    void updateVideoAdaptationSet();
    void updateAudioAdaptationSet();
    void updateAdaptationSetTimestamp();
    void updateVideoRepresentation();
    void updateAudioRepresentation();
    void removeRepresentation();

protected:
    MpdManager* manager = NULL;
};

void MpdManagerTest::setUp()
{
    manager = new MpdManager();
}

void MpdManagerTest::tearDown()
{
    delete manager;
}

void MpdManagerTest::writeToDisk()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;

    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("xmlns:xsi") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("xmlns") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("xmlns:xlink") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("xsi:schemaLocation") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("profiles") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("type") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("minimumUpdatePeriod") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("availabilityStartTime") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("timeShiftBufferDepth") != NULL);
    CPPUNIT_ASSERT(xmlRoot->FindAttribute("minBufferTime") != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("ProgramInformation")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("Title")) != NULL);
    CPPUNIT_ASSERT(std::string(xmlElement->GetText()) == std::string("Demo DASH"));

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT(xmlElement->FindAttribute("id") != NULL);
    CPPUNIT_ASSERT(xmlElement->FindAttribute("start") != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) == NULL);
}


void MpdManagerTest::configureTest()
{
    //TODO: test "invalid" values
    const size_t maxSeg = 10;
    const size_t minBufferTime = 6;
    const size_t segDuration = 2;
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLAttribute *xmlAttribute;
    const std::string sMinimumUpdatePeriod = "PT" + std::to_string(segDuration) + ".0S";
    const std::string sMinBufferTime = "PT" + std::to_string(minBufferTime) + ".0S";
    const std::string sTimeShiftBufferDepth = "PT" + std::to_string(segDuration * maxSeg) + ".0S";
    
    
    manager->configure(minBufferTime, maxSeg, segDuration);
    manager->writeToDisk(FILE_NAME);
    
    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);
    
    CPPUNIT_ASSERT((xmlAttribute = xmlRoot->FindAttribute("minimumUpdatePeriod")) != NULL);
    CPPUNIT_ASSERT(xmlAttribute->Value() == sMinimumUpdatePeriod);
    
    CPPUNIT_ASSERT((xmlAttribute = xmlRoot->FindAttribute("minBufferTime")) != NULL);
    CPPUNIT_ASSERT(xmlAttribute->Value() == sMinBufferTime);
    
    CPPUNIT_ASSERT((xmlAttribute = xmlRoot->FindAttribute("timeShiftBufferDepth")) != NULL);
    CPPUNIT_ASSERT(xmlAttribute->Value() == sTimeShiftBufferDepth);
}

void MpdManagerTest::updateVideoAdaptationSet()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;
    const tinyxml2::XMLAttribute *attrib;
    const std::string id = "one-id";
    const int timescale = 1;
    const std::string segmentTempl = "a-segment";
    const std::string initTempl = "the-init";
    const int updTimescale = 2;
    const std::string updSegmentTempl = "b-segment";
    const std::string updInitTempl = "other-init";

    manager->updateVideoAdaptationSet(id, timescale, segmentTempl, initTempl);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("timescale")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == timescale);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("media")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == segmentTempl);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("initialization")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == initTempl);

    //UPDATE

    manager->updateVideoAdaptationSet(id, updTimescale, updSegmentTempl, updInitTempl);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("timescale")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == updTimescale);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("media")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == updSegmentTempl);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("initialization")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == updInitTempl);
}

void MpdManagerTest::updateAudioAdaptationSet()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;
    const tinyxml2::XMLAttribute *attrib;
    const std::string id = "one-id";
    const int timescale = 1;
    const std::string segmentTempl = "a-segment";
    const std::string initTempl = "the-init";
    const int updTimescale = 2;
    const std::string updSegmentTempl = "b-segment";
    const std::string updInitTempl = "other-init";

    manager->updateAudioAdaptationSet(id, timescale, segmentTempl, initTempl);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("timescale")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == timescale);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("media")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == segmentTempl);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("initialization")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == initTempl);

    //UPDATE

    manager->updateAudioAdaptationSet(id, updTimescale, updSegmentTempl, updInitTempl);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("timescale")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == updTimescale);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("media")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == updSegmentTempl);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("initialization")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == updInitTempl);
}

void MpdManagerTest::updateAdaptationSetTimestamp()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;
    const tinyxml2::XMLElement *xmlSegment;
    const tinyxml2::XMLAttribute *attrib;
    const std::string id = "one-id";
    const int timescale = 1;
    const std::string segmentTempl = "a-segment";
    const std::string initTempl = "the-init";
    const int ts = 2;
    const int duration = 3;

    manager->updateAudioAdaptationSet(id, timescale, segmentTempl, initTempl);
    manager->updateAdaptationSetTimestamp(id, ts, duration);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTemplate")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("SegmentTimeline")) != NULL);

    CPPUNIT_ASSERT((xmlSegment = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("S")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlSegment->FindAttribute("t")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == ts);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlSegment->FindAttribute("d")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == duration);
}

void MpdManagerTest::updateVideoRepresentation()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;
    const tinyxml2::XMLAttribute *attrib;
    const std::string id = "one-id";
    const int updTimescale = 2;
    const std::string updSegmentTempl = "b-segment";
    const std::string updInitTempl = "other-init";
    const std::string representationId = "repr-Id";
    const std::string representationCodec = "repr-Codec";
    const int representationWidth = 2;
    const int representationHeight = 3;
    const int representationBandwidth = 4;
    const int representationFps = 5;

    manager->updateVideoAdaptationSet(id, updTimescale, updSegmentTempl, updInitTempl);
    manager->updateVideoRepresentation(id, representationId, representationCodec, representationWidth,
            representationHeight, representationBandwidth, representationFps);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("frameRate")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationFps);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("maxWidth")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationWidth);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("maxHeight")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationHeight);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("Representation")) != NULL);
    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("id");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationId);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("codecs");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationCodec);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("width");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationWidth);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("height");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationHeight);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("bandwidth");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationBandwidth);

    //Check maxWidth & maxHeight updating data with lower values

    manager->updateVideoRepresentation(id, representationId, representationCodec, representationWidth - 1,
            representationHeight - 1, representationBandwidth, representationFps);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("maxWidth")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationWidth);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("maxHeight")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationHeight);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("Representation")) != NULL);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("width");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationWidth - 1);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("height");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationHeight - 1);
}

void MpdManagerTest::updateAudioRepresentation()
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLElement *xmlRoot;
    const tinyxml2::XMLElement *xmlElement;
    const tinyxml2::XMLAttribute *attrib;
    const std::string id = "one-id";
    const int updTimescale = 2;
    const std::string updSegmentTempl = "b-segment";
    const std::string updInitTempl = "other-init";
    const std::string representationId = "repr-Id";
    const std::string representationCodec = "repr-Codec";
    const int representationSamplerate = 2;
    const int representationBandwidth = 4;
    const int representationChannels = 5;

    manager->updateAudioAdaptationSet(id, updTimescale, updSegmentTempl, updInitTempl);
    manager->updateAudioRepresentation(id, representationId, representationCodec, representationSamplerate,
            representationBandwidth, representationChannels);
    manager->writeToDisk(FILE_NAME);

    CPPUNIT_ASSERT(doc.LoadFile(FILE_NAME) == tinyxml2::XML_SUCCESS);

    CPPUNIT_ASSERT((xmlRoot = (const tinyxml2::XMLElement*)doc.FirstChildElement("MPD")) != NULL);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlRoot->FirstChildElement("Period")) != NULL);
    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AdaptationSet")) != NULL);
    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)xmlElement->FindAttribute("id")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == id);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("Representation")) != NULL);
    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("id");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationId);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("codecs");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationCodec);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("audioSamplingRate");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationSamplerate);

    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("bandwidth");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationBandwidth);

    CPPUNIT_ASSERT((xmlElement = (const tinyxml2::XMLElement*)xmlElement->FirstChildElement("AudioChannelConfiguration")) != NULL);
    attrib = ((const tinyxml2::XMLElement*)xmlElement)->FindAttribute("value");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationChannels);
}

void MpdManagerTest::removeRepresentation()
{
    const std::string vAdSetId = "v-ad-id";
    const std::string aAdSetId = "a-ad-id";
    const int updTimescale = 2;
    const std::string updSegmentTempl = "b-segment";
    const std::string updInitTempl = "other-init";
    const std::string audioReprId = "audioReprId";
    const std::string videoReprId = "videoReprId";
    const std::string representationCodec = "repr-Codec";
    const int representationWidth = 2;
    const int representationHeight = 3;
    const int representationBandwidth = 4;
    const int representationFps = 5;
    const int representationSamplerate = 2;
    const int representationChannels = 5;

    manager->updateAudioAdaptationSet(aAdSetId, updTimescale, updSegmentTempl, updInitTempl);
    manager->updateAudioRepresentation(aAdSetId, audioReprId, representationCodec, representationSamplerate,
            representationBandwidth, representationChannels);

    manager->updateVideoAdaptationSet(vAdSetId, updTimescale, updSegmentTempl, updInitTempl);
    manager->updateVideoRepresentation(vAdSetId, videoReprId, representationCodec, representationWidth,
            representationHeight, representationBandwidth, representationFps);

    CPPUNIT_ASSERT(!manager->removeRepresentation(aAdSetId, videoReprId));
    CPPUNIT_ASSERT(!manager->removeRepresentation(vAdSetId, audioReprId));
    CPPUNIT_ASSERT(manager->removeRepresentation(vAdSetId, videoReprId));
    CPPUNIT_ASSERT(manager->removeRepresentation(aAdSetId, audioReprId));
    CPPUNIT_ASSERT(!manager->removeRepresentation(vAdSetId, videoReprId));
    CPPUNIT_ASSERT(!manager->removeRepresentation(aAdSetId, audioReprId));
}

class AdaptationSetTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AdaptationSetTest);
    CPPUNIT_TEST(updateTest);
    CPPUNIT_TEST(updateTimestampTest);
    CPPUNIT_TEST_SUITE_END();

protected:
    void updateTest();
    void updateTimestampTest();

protected:
    const int iSegTimescale = 100;
    const std::string sSegTempl = "segTemp";
    const std::string sInitTempl = "initTemp";

    AdaptationSet* as = NULL;
};

void AdaptationSetTest::updateTest()
{
    CPPUNIT_ASSERT(as != NULL);

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement *segmentTemplate;
    const tinyxml2::XMLAttribute* attrib;
    const int iNewTimescale = 200;
    const std::string sNewSegTempl = "newSegTemp";
    const std::string sNewInitTempl = "newInitTemp";

    adaptSet = doc.NewElement("newElement");

    as->update(iNewTimescale, sNewSegTempl, sNewInitTempl);

    as->toMpd(doc, adaptSet);

    CPPUNIT_ASSERT((segmentTemplate = (const tinyxml2::XMLElement*)adaptSet->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)segmentTemplate->FindAttribute("timescale")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == iNewTimescale);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)segmentTemplate->FindAttribute("media")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == sNewSegTempl);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)segmentTemplate->FindAttribute("initialization")) != NULL);
    CPPUNIT_ASSERT(attrib->Value() == sNewInitTempl);
}

void AdaptationSetTest::updateTimestampTest()
{
    CPPUNIT_ASSERT(as != NULL);

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement *segmentTemplate;
    const tinyxml2::XMLElement *segmentTimeline;
    const tinyxml2::XMLElement *segment;
    const tinyxml2::XMLAttribute* attrib;
    const int iTimestamp = 1;
    const int iDuration = 2;

    adaptSet = doc.NewElement("newElement");

    as->updateTimestamp(iTimestamp, iDuration, MAX_SEGMENTS);

    as->toMpd(doc, adaptSet);

    CPPUNIT_ASSERT((segmentTemplate = (const tinyxml2::XMLElement*)adaptSet->FirstChildElement("SegmentTemplate")) != NULL);
    CPPUNIT_ASSERT((segmentTimeline = (const tinyxml2::XMLElement*)segmentTemplate->FirstChildElement("SegmentTimeline")) != NULL);
    CPPUNIT_ASSERT((segment = (const tinyxml2::XMLElement*)segmentTimeline->FirstChildElement("S")) != NULL);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)segment->FindAttribute("t")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == iTimestamp);

    CPPUNIT_ASSERT((attrib = (const tinyxml2::XMLAttribute *)segment->FindAttribute("d")) != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == iDuration);
}

class VideoAdaptationSetTest : public AdaptationSetTest
{
    CPPUNIT_TEST_SUB_SUITE(VideoAdaptationSetTest, AdaptationSetTest);
    CPPUNIT_TEST(toMpdTest);
    CPPUNIT_TEST(updateVideoRepresentationTest);
    CPPUNIT_TEST(removeRepresentationTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void toMpdTest();
    void updateVideoRepresentationTest();
    void removeRepresentationTest();
};

void VideoAdaptationSetTest::setUp()
{
    as = new VideoAdaptationSet(iSegTimescale, sSegTempl, sInitTempl);
}

void VideoAdaptationSetTest::tearDown()
{
    delete as;
}

void VideoAdaptationSetTest::toMpdTest()
{
    int nAttribs = 0;
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement *segmentTemplate;
    const tinyxml2::XMLAttribute* attrib;

    adaptSet = doc.NewElement("newElement");
    as->toMpd(doc, adaptSet);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("noValidAttribute") == NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("mimeType") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("maxWidth") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("maxHeight") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("frameRate") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("segmentAlignment") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("startWithSAP") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("subsegmentAlignment") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("subsegmentStartsWithSAP") != NULL);

    attrib = adaptSet->FirstAttribute();
    for (nAttribs = 0 ; attrib ; nAttribs++) {
        attrib = attrib->Next();
    }

    CPPUNIT_ASSERT(nAttribs == 8);

    CPPUNIT_ASSERT((segmentTemplate = (const tinyxml2::XMLElement*)adaptSet->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("timescale") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("media") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("initialization") != NULL);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FirstChildElement("SegmentTimeline") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FirstChildElement("Representation") == NULL);
}

void VideoAdaptationSetTest::updateVideoRepresentationTest()
{
    const std::string representationName = "Representation";
    const std::string representationId = "my-id";
    const std::string representationCodec = "my-codec";
    const int representationWidth = 1;
    const int representationHeight = 2;
    const int representationBandwidth = 3;
    const int representationFps = 4;
    const std::string audioChannelConfigurationName = "AudioChannelConfiguration";

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement* repElem;
    const tinyxml2::XMLAttribute* attrib;

    adaptSet = doc.NewElement("newElement");

    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);

    as->updateVideoRepresentation(representationId, representationCodec, representationWidth, representationHeight,
            representationBandwidth, representationFps);

    as->toMpd(doc, adaptSet);
    repElem = adaptSet->FirstChildElement(representationName.c_str());

    CPPUNIT_ASSERT(repElem != NULL);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("id");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationId);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("codecs");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationCodec);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("width");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationWidth);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("height");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationHeight);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("bandwidth");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationBandwidth);
}

void VideoAdaptationSetTest::removeRepresentationTest()
{
    const std::string representationName = "Representation";
    const std::string representationId = "my-id";
    const std::string representationCodec = "my-codec";
    const int representationWidth = 1;
    const int representationHeight = 2;
    const int representationBandwidth = 3;
    const int representationFps = 4;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement* repElem;

    adaptSet = doc.NewElement("newElement");

    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);

    as->updateVideoRepresentation(representationId, representationCodec, representationWidth, representationHeight,
            representationBandwidth, representationFps);

    as->toMpd(doc, adaptSet);
    repElem = adaptSet->FirstChildElement(representationName.c_str());

    CPPUNIT_ASSERT(repElem != NULL);
    CPPUNIT_ASSERT(as->removeRepresentation(representationId));

    adaptSet = doc.NewElement("newElement");
    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);
}

class AudioAdaptationSetTest : public AdaptationSetTest
{
    CPPUNIT_TEST_SUB_SUITE(AudioAdaptationSetTest, AdaptationSetTest);
    CPPUNIT_TEST(toMpdTest);
    CPPUNIT_TEST(updateAudioRepresentationTest);
    CPPUNIT_TEST(removeRepresentationTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void toMpdTest();
    void updateAudioRepresentationTest();
    void removeRepresentationTest();
};

void AudioAdaptationSetTest::setUp()
{
    as = new AudioAdaptationSet(iSegTimescale, sSegTempl, sInitTempl);
}

void AudioAdaptationSetTest::tearDown()
{
    delete as;
}

void AudioAdaptationSetTest::toMpdTest()
{
    int nAttribs = 0;
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement *segmentTemplate, *role;
    const tinyxml2::XMLAttribute* attrib;

    adaptSet = doc.NewElement("newElement");
    as->toMpd(doc, adaptSet);
    doc.SaveFile(FILE_NAME);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("noValidAttribute") == NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("mimeType") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("lang") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("segmentAlignment") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("startWithSAP") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("subsegmentAlignment") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FindAttribute("subsegmentStartsWithSAP") != NULL);

    attrib = adaptSet->FirstAttribute();
    for (nAttribs = 0 ; attrib ; nAttribs++) {
        attrib = attrib->Next();
    }

    CPPUNIT_ASSERT(nAttribs == 6);

    CPPUNIT_ASSERT((role = (const tinyxml2::XMLElement*)adaptSet->FirstChildElement("Role")) != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)role)->FindAttribute("schemeIdUri") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)role)->FindAttribute("value") != NULL);

    CPPUNIT_ASSERT((segmentTemplate = (const tinyxml2::XMLElement*)adaptSet->FirstChildElement("SegmentTemplate")) != NULL);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("timescale") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("media") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FindAttribute("initialization") != NULL);

    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)segmentTemplate)->FirstChildElement("SegmentTimeline") != NULL);
    CPPUNIT_ASSERT(((const tinyxml2::XMLElement*)adaptSet)->FirstChildElement("Representation") == NULL);
}

void AudioAdaptationSetTest::updateAudioRepresentationTest()
{
    const std::string representationName = "Representation";
    const std::string representationId = "my-id";
    const std::string representationCodec = "my-codec";
    const int representationAudioSamplingRate = 1;
    const int representationBandwidth = 2;
    const int representationChannels = 3;
    const std::string audioChannelConfigurationName = "AudioChannelConfiguration";

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement* repElem, *audElem;
    const tinyxml2::XMLAttribute* attrib;

    adaptSet = doc.NewElement("newElement");

    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);

    as->updateAudioRepresentation(representationId, representationCodec, representationAudioSamplingRate,
            representationBandwidth, representationChannels);

    as->toMpd(doc, adaptSet);
    repElem = adaptSet->FirstChildElement(representationName.c_str());

    CPPUNIT_ASSERT(repElem != NULL);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("id");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationId);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("codecs");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->Value() == representationCodec);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("audioSamplingRate");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationAudioSamplingRate);

    attrib = ((const tinyxml2::XMLElement*)repElem)->FindAttribute("bandwidth");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationBandwidth);

    audElem = repElem->FirstChildElement(audioChannelConfigurationName.c_str());

    CPPUNIT_ASSERT(audElem != NULL);

    attrib = ((const tinyxml2::XMLElement*)audElem)->FindAttribute("value");
    CPPUNIT_ASSERT(attrib != NULL);
    CPPUNIT_ASSERT(attrib->IntValue() == representationChannels);
}

void AudioAdaptationSetTest::removeRepresentationTest()
{
    const std::string representationName = "Representation";
    const std::string representationId = "my-id";
    const std::string representationCodec = "my-codec";
    const int representationAudioSamplingRate = 1;
    const int representationBandwidth = 2;
    const int representationChannels = 3;
    const std::string audioChannelConfigurationName = "AudioChannelConfiguration";

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* adaptSet;
    const tinyxml2::XMLElement* repElem;

    adaptSet = doc.NewElement("newElement");

    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);

    as->updateAudioRepresentation(representationId, representationCodec, representationAudioSamplingRate,
            representationBandwidth, representationChannels);

    as->toMpd(doc, adaptSet);
    repElem = adaptSet->FirstChildElement(representationName.c_str());

    CPPUNIT_ASSERT(repElem != NULL);
    CPPUNIT_ASSERT(as->removeRepresentation(representationId));

    adaptSet = doc.NewElement("newElement");
    as->toMpd(doc, adaptSet);
    CPPUNIT_ASSERT(adaptSet->FirstChildElement(representationName.c_str()) == NULL);
}

class VideoRepresentationTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VideoRepresentationTest);
    CPPUNIT_TEST(updateTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void updateTest();

protected:
    const std::string sInitCodec = "null";
    const int iInitWidth = 1;
    const int iInitHeight = 2;
    const int iInitBandwidth = 3;

    VideoRepresentation* vRep = NULL;
};

void VideoRepresentationTest::setUp()
{
    vRep = new VideoRepresentation(sInitCodec, iInitWidth, iInitHeight, iInitBandwidth);
}

void VideoRepresentationTest::tearDown()
{
    delete vRep;
}

void VideoRepresentationTest::updateTest()
{
    const std::string sCodec = "codec";
    const int iWidth = 100;
    const int iHeight = 200;
    const int iBandwidth = 300;

    CPPUNIT_ASSERT(vRep->getCodec() == sInitCodec);
    CPPUNIT_ASSERT(vRep->getWidth() == iInitWidth);
    CPPUNIT_ASSERT(vRep->getHeight() == iInitHeight);
    CPPUNIT_ASSERT(vRep->getBandwidth() == iInitBandwidth);

    vRep->update(sCodec, iWidth, iHeight, iBandwidth);

    CPPUNIT_ASSERT(vRep->getCodec() == sCodec);
    CPPUNIT_ASSERT(vRep->getWidth() == iWidth);
    CPPUNIT_ASSERT(vRep->getHeight() == iHeight);
    CPPUNIT_ASSERT(vRep->getBandwidth() == iBandwidth);
}

class AudioRepresentationTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(AudioRepresentationTest);
    CPPUNIT_TEST(updateTest);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void updateTest();

protected:
    const std::string sInitCodec = "null";
    const int iInitSampleRate = 1;
    const int iInitBandwidth = 2;
    const int iInitChannels = 3;

    AudioRepresentation* aRep = NULL;
};

void AudioRepresentationTest::setUp()
{
    aRep = new AudioRepresentation(sInitCodec, iInitSampleRate, iInitBandwidth, iInitChannels);
}

void AudioRepresentationTest::tearDown()
{
    delete aRep;
}

void AudioRepresentationTest::updateTest()
{
    const std::string sCodec = "codec";
    const int iSampleRate = 100;
    const int iBandwidth = 200;
    const int iChannels = 300;

    CPPUNIT_ASSERT(aRep->getCodec() == sInitCodec);
    CPPUNIT_ASSERT(aRep->getSampleRate() == iInitSampleRate);
    CPPUNIT_ASSERT(aRep->getBandwidth() == iInitBandwidth);
    CPPUNIT_ASSERT(aRep->getAudioChannelConfigValue() == iInitChannels);

    aRep->update(sCodec, iSampleRate, iBandwidth, iChannels);

    CPPUNIT_ASSERT(aRep->getCodec() == sCodec);
    CPPUNIT_ASSERT(aRep->getSampleRate() == iSampleRate);
    CPPUNIT_ASSERT(aRep->getBandwidth() == iBandwidth);
    CPPUNIT_ASSERT(aRep->getAudioChannelConfigValue() == iChannels);
}

CPPUNIT_TEST_SUITE_REGISTRATION( MpdManagerTest );
CPPUNIT_TEST_SUITE_REGISTRATION( VideoAdaptationSetTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioAdaptationSetTest );
CPPUNIT_TEST_SUITE_REGISTRATION( VideoRepresentationTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioRepresentationTest );

int main(int argc, char* argv[])
{
    std::ofstream xmlout("MpdManagerTestResult.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    return runner.result().wasSuccessful() ? 0 : 1;
}
