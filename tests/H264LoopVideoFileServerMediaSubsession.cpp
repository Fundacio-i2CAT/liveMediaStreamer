#include "H264LoopVideoFileServerMediaSubsession.hh"
#include "LoopByteStreamFileSource.hh"

H264LoopVideoFileServerMediaSubsession::H264LoopVideoFileServerMediaSubsession(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource) :
    H264VideoFileServerMediaSubsession(env, fileName, reuseFirstSource)
{
}

H264LoopVideoFileServerMediaSubsession*
H264LoopVideoFileServerMediaSubsession::createNew(UsageEnvironment& env,
                                              char const* fileName,
                                              Boolean reuseFirstSource) {
  return new H264LoopVideoFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

FramedSource* H264LoopVideoFileServerMediaSubsession::createNewStreamSource(unsigned clientSessionId,
                                    unsigned& estBitrate)
{
    int reset = 0;
    estBitrate = 500; // kbps, estimate

    // Create the video source:
    LoopByteStreamFileSource* fileSource = LoopByteStreamFileSource::createNew(envir(), fFileName, reset);
    if (fileSource == NULL) return NULL;
    fFileSize = fileSource->fileSize();

    // Create a framer for the Video Elementary Stream:
    return H264VideoStreamFramer::createNew(envir(), fileSource);
}


