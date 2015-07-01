/*
 *  EncodingDecodingTest.cpp - EncodingDecodingFunctionalTest class test
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>
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
#include "modules/videoEncoder/VideoEncoderX264.hh"
#include "modules/videoEncoder/VideoEncoderX265.hh"
#include "modules/videoDecoder/VideoDecoderLibav.hh"

#define VIDEO_FRAMES    1000
#define BITS_X_BYTE     8

class VideoEncoderDecoderFunctionalTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VideoEncoderDecoderFunctionalTest);
    CPPUNIT_TEST(test500);
    CPPUNIT_TEST(test2000);
    CPPUNIT_TEST(test4000);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

protected:
    void test500();
    void test2000();
    void test4000();

    OneToOneVideoScenarioMockup *x264encodingSce, *x265encodingSce, *x264decodingSce, *x265decodingSce;
    VideoEncoderX264or5* x264encoder;
    VideoEncoderX264or5* x265encoder;
    VideoDecoderLibav* x264decoder;
    VideoDecoderLibav* x265decoder;
    AVFramesReader* reader;
    InterleavedFramesWriter* writer;
};

void VideoEncoderDecoderFunctionalTest::setUp()
{
    x264encoder = new VideoEncoderX264();
    x265encoder = new VideoEncoderX265();
    x264decoder = new VideoDecoderLibav();
    x265decoder = new VideoDecoderLibav();
    x264encodingSce = new OneToOneVideoScenarioMockup(x264encoder, RAW, YUV420P);
    x265encodingSce = new OneToOneVideoScenarioMockup(x265encoder, RAW, YUV420P);
    x264decodingSce = new OneToOneVideoScenarioMockup(x264decoder, H264);
    x265decodingSce = new OneToOneVideoScenarioMockup(x265decoder, H265);
    CPPUNIT_ASSERT(x264encodingSce->connectFilter());
    CPPUNIT_ASSERT(x265encodingSce->connectFilter());
    CPPUNIT_ASSERT(x264decodingSce->connectFilter());
    CPPUNIT_ASSERT(x265decodingSce->connectFilter());
    reader = new AVFramesReader();
    writer = new InterleavedFramesWriter();
}

void VideoEncoderDecoderFunctionalTest::tearDown()
{
    x264encodingSce->disconnectFilter();
    x265encodingSce->disconnectFilter();
    x264decodingSce->disconnectFilter();
    x265decodingSce->disconnectFilter();
    delete x264encodingSce;
    delete x265encodingSce;
    delete x264decodingSce;
    delete x265decodingSce;
    delete x264encoder;
    delete x265encoder;
    delete x264decoder;
    delete x265decoder;
    delete reader;
    delete writer;
}

void VideoEncoderDecoderFunctionalTest::test500()
{
    InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame *midFrame = NULL;
    InterleavedVideoFrame *filteredFrame = NULL;
    bool milestone = false;
    size_t fileSize;
    
    CPPUNIT_ASSERT(x264encoder->configure(500, 0, DEFAULT_GOP, 0, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    CPPUNIT_ASSERT(x265encoder->configure(500, 0, DEFAULT_GOP, 0, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    
    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest.h264", H264));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_500kbps.h264"));
    
    while((frame = reader->getFrame())!=NULL){
        x264decodingSce->processFrame(frame);
        while ((midFrame = x264decodingSce->extractFrame())){
            x264encodingSce->processFrame(midFrame);
            while((filteredFrame = x264encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*500*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*500*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);

    milestone = false;
    frame = NULL;
    midFrame = NULL;
    filteredFrame = NULL;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest_out_500kbps.h264", H264));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_500kbps.h265"));

    while((frame = reader->getFrame())!=NULL){
        x264decodingSce->processFrame(frame);
        while ((midFrame = x264decodingSce->extractFrame())){
            x265encodingSce->processFrame(midFrame);
            while((filteredFrame = x265encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();

    std::cout << "Filesize:" << writer->getFileSize() << std::endl;
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*500*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*500*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);

}

void VideoEncoderDecoderFunctionalTest::test2000()
{
    InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame *midFrame = NULL;
    InterleavedVideoFrame *filteredFrame = NULL;
    bool milestone = false;
    size_t fileSize;
    
    //TODO test with lookahead = 2xDEFAULT_GOP --> to implement flush encoder method!
    CPPUNIT_ASSERT(x264encoder->configure(2000, 0, DEFAULT_GOP, DEFAULT_GOP, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    CPPUNIT_ASSERT(x265encoder->configure(2000, 0, DEFAULT_GOP, DEFAULT_GOP, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    
    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest.hevc", H265));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_2000kbps.h264"));
    
    while((frame = reader->getFrame())!=NULL){
        x265decodingSce->processFrame(frame);
        while ((midFrame = x265decodingSce->extractFrame())){
            x264encodingSce->processFrame(midFrame);
            while((filteredFrame = x264encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*2000*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*2000*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);

    milestone = false;
    frame = NULL;
    midFrame = NULL;
    filteredFrame = NULL;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest_out_2000kbps.h264", H264));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_2000kbps.h265"));

    while((frame = reader->getFrame())!=NULL){
        x264decodingSce->processFrame(frame);
        while ((midFrame = x264decodingSce->extractFrame())){
            x265encodingSce->processFrame(midFrame);
            while((filteredFrame = x265encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*2000*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*2000*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);
}

void VideoEncoderDecoderFunctionalTest::test4000()
{
    InterleavedVideoFrame *frame = NULL;
    InterleavedVideoFrame *midFrame = NULL;
    InterleavedVideoFrame *filteredFrame = NULL;
    bool milestone = false;
    size_t fileSize;
    
    CPPUNIT_ASSERT(x264encoder->configure(4000, 0, DEFAULT_GOP, DEFAULT_GOP, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    CPPUNIT_ASSERT(x265encoder->configure(4000, 0, DEFAULT_GOP, DEFAULT_GOP, 
                       DEFAULT_THREADS, DEFAULT_ANNEXB, DEFAULT_PRESET));
    
    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest.h264", H264));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_4000kbps.h264"));
    
    while((frame = reader->getFrame())!=NULL){
        x264decodingSce->processFrame(frame);
        while ((midFrame = x264decodingSce->extractFrame())){
            x264encodingSce->processFrame(midFrame);
            while((filteredFrame = x264encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*4000*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*4000*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);

    milestone = false;
    frame = NULL;
    midFrame = NULL;
    filteredFrame = NULL;

    CPPUNIT_ASSERT(reader->openFile("testsData/videoVectorTest_out_4000kbps.h264", H264));
    CPPUNIT_ASSERT(writer->openFile("testsData/videoVectorTest_out_4000kbps.h265"));

    while((frame = reader->getFrame())!=NULL){
        x264decodingSce->processFrame(frame);
        while ((midFrame = x264decodingSce->extractFrame())){
            x265encodingSce->processFrame(midFrame);
            while((filteredFrame = x265encodingSce->extractFrame())){
                CPPUNIT_ASSERT(filteredFrame->getWidth() == midFrame->getWidth() 
                    && filteredFrame->getHeight() == midFrame->getHeight());
                CPPUNIT_ASSERT(writer->writeInterleavedFrame(filteredFrame));
                milestone = true;
            }
        }
    }  
    
    writer->closeFile();
    reader->close();
        
    CPPUNIT_ASSERT((fileSize = writer->getFileSize()) > 0);
    
    CPPUNIT_ASSERT(VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*4000*1000*1.1 > fileSize*BITS_X_BYTE &&
        VIDEO_FRAMES/VIDEO_DEFAULT_FRAMERATE*4000*1000*0.9 < fileSize*BITS_X_BYTE
    );
    
    CPPUNIT_ASSERT(milestone);
}

CPPUNIT_TEST_SUITE_REGISTRATION(VideoEncoderDecoderFunctionalTest);

int main(int argc, char* argv[])
{
    std::ofstream xmlout("VideoEncoderDecoderFunctionalTest.xml");
    CPPUNIT_NS::TextTestRunner runner;
    CPPUNIT_NS::XmlOutputter *outputter = new CPPUNIT_NS::XmlOutputter(&runner.result(), xmlout);

    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( "", false );
    outputter->write();

    utils::printMood(runner.result().wasSuccessful());

    return runner.result().wasSuccessful() ? 0 : 1;
}
