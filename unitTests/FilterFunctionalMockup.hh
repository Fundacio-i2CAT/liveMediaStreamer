/*
 *  FilterFunctionalMockup - OneToOne Filter scenario test
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors:    David Cassany <david.cassany@i2cat.net>
 *
 */

#ifndef _FILTER_FUNCTIONAL_MOCKUP_HH
#define _FILTER_FUNCTIONAL_MOCKUP_HH

#include <chrono>
#include <fstream>
#include <sys/stat.h>

extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/pixdesc.h>
}

#include "Filter.hh"
#include "VideoFrame.hh"
#include "FilterMockup.hh"

static AVPixelFormat getPixelFormat(PixType format);
static AVCodecID getCodec(VCodecType codec);

class OneToOneVideoScenarioMockup {
public:
    OneToOneVideoScenarioMockup(OneToOneFilter* fToTest, VCodecType c, PixType pix = P_NONE): filterToTest(fToTest){
        headF = new VideoHeadFilterMockup(c, pix);
        tailF = new VideoTailFilterMockup();
    };

    ~OneToOneVideoScenarioMockup(){
        disconnectFilter();
        delete headF;
        delete tailF;
    }

    bool connectFilter(){
        if (filterToTest == NULL){
            return false;
        }

        if (! headF->connectOneToOne(filterToTest)){
            return false;
        }

        if (! filterToTest->connectOneToOne(tailF)){
            return false;
        }

        return true;
    }

    void disconnectFilter(){
        CPPUNIT_ASSERT(headF->disconnectWriter(1));
        CPPUNIT_ASSERT(filterToTest->disconnectWriter(1));
        CPPUNIT_ASSERT(filterToTest->disconnectReader(1));
        CPPUNIT_ASSERT(tailF->disconnectReader(1));
    }

    int processFrame(InterleavedVideoFrame* srcFrame){
        int ret;

        if (! headF->inject(srcFrame)){
            return 0;
        }
        headF->processFrame(ret);
        filterToTest->processFrame(ret);
        return ret;
    }

    InterleavedVideoFrame *extractFrame(){
        int ret;
        tailF->processFrame(ret);
        return tailF->extract();
    }

private:
    VideoHeadFilterMockup *headF;
    VideoTailFilterMockup *tailF;
    OneToOneFilter *filterToTest;
};

class ManyToOneVideoScenarioMockup {

public:
    ManyToOneVideoScenarioMockup(ManyToOneFilter* fToTest): filterToTest(fToTest)
    {
        tailF = new VideoTailFilterMockup();
    };

    ~ManyToOneVideoScenarioMockup()
    {
        for (auto f : headFilters) {
            delete f.second;
        }

        delete tailF;
    }

    bool addHeadFilter(int id, VCodecType c, PixType pix = P_NONE)
    {
        if (headFilters.count(id) > 0) {
            return false;
        }

        headFilters[id] = new VideoHeadFilterMockup(c, pix);
        return true;
    }

    bool connectFilters()
    {
        if (filterToTest == NULL || headFilters.empty()) {
            return false;
        }

        for (auto f : headFilters) {
            if (!f.second->connectOneToMany(filterToTest, f.first)) {
                return false;
            }
        }

        if (!filterToTest->connectOneToOne(tailF)) {
            return false;
        }

        return true;
    };

    int processFrame(InterleavedVideoFrame* srcFrame)
    {
        int ret;

        for (auto f : headFilters) {
            if (!f.second->inject(srcFrame)) {
                return 0;
            }
            f.second->processFrame(ret);
        }

        filterToTest->processFrame(ret);
        return ret;
    }

    InterleavedVideoFrame *extractFrame()
    {
        int ret;
        tailF->processFrame(ret);
        return tailF->extract();
    }

private:

    std::map<int,VideoHeadFilterMockup*> headFilters;
    ManyToOneFilter *filterToTest;
    VideoTailFilterMockup *tailF;
};

class OneToManyVideoScenarioMockup {

public:
    OneToManyVideoScenarioMockup(OneToManyFilter* fToTest, VCodecType c, PixType pix = P_NONE): filterToTest(fToTest){
        headF = new VideoHeadFilterMockup(c, pix);
    };

    ~OneToManyVideoScenarioMockup()
    {
        delete headF;
        for (auto f : tailFilters) {
            delete f.second;
        }
    }

    bool addTailFilter(int id)
    {
        if(tailFilters.count(id) > 0) {
            return false;
        }

        tailFilters[id] = new VideoTailFilterMockup();
        return true;
    }

    bool connectFilters()
    {

        if (filterToTest == NULL || tailFilters.empty()) {
            return false;
        }

        if (! headF->connectOneToOne(filterToTest)){
            return false;
        }

        for (auto f : tailFilters) {
            if (!filterToTest->connectManyToOne(f.second, f.first)) {
                return false;
            }
        }
        return true;
    };

    int processFrame(InterleavedVideoFrame* srcFrame){
        int ret;

        if (!headF->inject(srcFrame)){
            return 0;
        }
        headF->processFrame(ret);
        filterToTest->processFrame(ret);
        return ret;
    }

    InterleavedVideoFrame *extractFrame(int id)
    {
        int ret;
        if(tailFilters.count(id) > 0 ){
            tailFilters[id]->processFrame(ret);
            return tailFilters[id]->extract();
        }

        return NULL;
    }

private:
    std::map<int,VideoTailFilterMockup*> tailFilters;
    OneToManyFilter *filterToTest;
    VideoHeadFilterMockup *headF;
};

class ManyToOneAudioScenarioMockup {

public:
    ManyToOneAudioScenarioMockup(ManyToOneFilter* fToTest): filterToTest(fToTest)
    {
        tailF = new AudioTailFilterMockup();
    };

    ~ManyToOneAudioScenarioMockup()
    {

        for (auto f : headFilters) {
            delete f.second;
        }

        delete tailF;
    }

    bool addHeadFilter(int id, int channels, int sampleRate, SampleFmt sampleFormat)
    {
        if (headFilters.count(id) > 0) {
            return false;
        }

        headFilters[id] = new AudioHeadFilterMockup(channels, sampleRate, sampleFormat);
        return true;
    }

    bool connectFilters()
    {
        if (filterToTest == NULL || headFilters.empty()) {
            return false;
        }

        for (auto f : headFilters) {
            if (!f.second->connectOneToMany(filterToTest, f.first)) {
                return false;
            }
        }

        if (!filterToTest->connectOneToOne(tailF)) {
            return false;
        }

        return true;
    };

    int processFrame(PlanarAudioFrame* srcFrame)
    {
        int ret;

        for (auto f : headFilters) {
            if (!f.second->inject(srcFrame)) {
                return 0;
            }
            f.second->processFrame(ret);
        }

        filterToTest->processFrame(ret);
        return ret;
    }

    PlanarAudioFrame *extractFrame()
    {
        int ret;
        tailF->processFrame(ret);
        return tailF->extract();
    }

private:
    std::map<int,AudioHeadFilterMockup*> headFilters;
    ManyToOneFilter *filterToTest;
    AudioTailFilterMockup *tailF;
};

class InterleavedFramesWriter {
public:
    InterleavedFramesWriter(): file(""){};

    bool openFile(std::string fileName){
        outfile.open(fileName, std::ofstream::binary);
        if(outfile.is_open()){
            file = fileName;
            return true;
        }

        return false;
    }

    void closeFile(){
        if (outfile.is_open()){
            outfile.close();
        }
    }

    size_t getFileSize(){
        struct stat buffer;
        int rc;

        if (!file.empty()){
            rc = stat(file.c_str(), &buffer);
            return rc == 0 ? buffer.st_size : 0;
        }

        return 0;
    }

    bool writeInterleavedFrame(InterleavedVideoFrame *frame){
        if (outfile.is_open() && frame != NULL){
            outfile.write((char*)frame->getDataBuf(), frame->getLength());
            return true;
        }

        return false;
    }

private:
    std::ofstream outfile;
    std::string file;
};

class AVFramesReader {
public:
    AVFramesReader(): fmtCtx(NULL), frame(NULL){
        av_register_all();
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
    };

    ~AVFramesReader() {
        close();
    };

    bool openFile(std::string file, VCodecType c, PixType pix = P_NONE,
                  unsigned int width = 0, unsigned int height = 0){
        int ret;
        struct stat buffer;

        if (stat (file.c_str(), &buffer) != 0){
            return false;
        }

        if (fmtCtx){
            return false;
        }

        fmtCtx = avformat_alloc_context();

        AVInputFormat* inputFormat = av_find_input_format(avcodec_get_name(getCodec(c)));
        if (!inputFormat){
            return false;
        }

        AVDictionary *options = NULL;
        if (width != 0 && height != 0){
            std::string videoSize = std::to_string(width) + "x" + std::to_string(height);
            av_dict_set(&options, "video_size", videoSize.c_str(), 0);
        }
        if (pix != P_NONE){
            av_dict_set(&options, "pixel_format", av_get_pix_fmt_name(getPixelFormat(pix)), 0);
        }

        ret = avformat_open_input(&fmtCtx, file.c_str(), inputFormat, &options);
        if (ret < 0) {
            av_dict_free(&options);
            avformat_free_context(fmtCtx);
            fmtCtx = NULL;
            return false;
        }

        if (width != 0 && height != 0){
            frame = InterleavedVideoFrame::createNew(c, width, height, pix);
        } else {
            frame = InterleavedVideoFrame::createNew(c, DEFAULT_WIDTH, DEFAULT_HEIGHT, pix);
        }

        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        return true;
    };

    void close(){
        if (fmtCtx){
            avformat_close_input(&fmtCtx);
        }

        if (frame){
            delete frame;
            frame = NULL;
        }

        av_free_packet(&pkt);
    }

    InterleavedVideoFrame* getFrame(){
        if (!fmtCtx || !frame){
            return NULL;
        }

        if (av_read_frame(fmtCtx, &pkt) >= 0){
            memmove(frame->getDataBuf(), pkt.data, sizeof(unsigned char)*pkt.size);
            frame->setLength(pkt.size);
            frame->setPresentationTime(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()));
            return frame;
        }

        return NULL;
    }

private:
    AVFormatContext *fmtCtx;
    AVPacket pkt;
    InterleavedVideoFrame* frame;
};

static AVPixelFormat getPixelFormat(PixType format)
{
    switch(format){
        case RGB24:
            return AV_PIX_FMT_RGB24;
            break;
        case RGB32:
            return AV_PIX_FMT_RGB32;
            break;
        case YUYV422:
            return AV_PIX_FMT_YUYV422;
            break;
        case YUV420P:
            return AV_PIX_FMT_YUV420P;
            break;
        case YUV422P:
            return AV_PIX_FMT_YUV422P;
            break;
        case YUV444P:
            return AV_PIX_FMT_YUV444P;
            break;
        case YUVJ420P:
            return AV_PIX_FMT_YUVJ420P;
            break;
        default:
            utils::errorMsg("Unknown output pixel format");
            break;
    }

    return AV_PIX_FMT_NONE;
}

static AVCodecID getCodec(VCodecType codec)
{
    switch(codec){
        case H264:
            return AV_CODEC_ID_H264;
            break;
        case H265:
            return AV_CODEC_ID_HEVC;
            break;
        case VP8:
            return AV_CODEC_ID_VP8;
            break;
        case MJPEG: //TODO
            return AV_CODEC_ID_MJPEG;
            break;
        case RAW:
            return AV_CODEC_ID_RAWVIDEO ;
        default:
            utils::errorMsg("Codec not supported");
            break;
    }

    return AV_CODEC_ID_NONE;
};

#endif
