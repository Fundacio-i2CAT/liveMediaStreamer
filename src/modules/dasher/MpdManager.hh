/*
 *  MpdManager - DASH MPD manager class
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors: Xavier Carol <xavier.carol@i2cat.net>
 *           Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _MPD_MANAGER_HH_
#define _MPD_MANAGER_HH_

#include <map>
#include <deque>
#include <tinyxml2.h>

#define MAX_SEGMENTS_IN_MPD 6
#define XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XMLNS "urn:mpeg:dash:schema:mpd:2011"
#define XMLNS_XLINK "http://www.w3.org/1999/xlink"
#define XSI_SCHEMA_LOCATION "urn:mpeg:DASH:schema:MPD:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd"
#define PROFILES "urn:mpeg:dash:profile:isoff-live:2011"
#define AVAILABILITY_START_TIME "2014-10-29T03:07:39"
#define TYPE_DYNAMIC "dynamic"
#define PERIOD_ID 0
#define PERIOD_START "PT0.0S"
#define SEGMENT_ALIGNMENT true
#define START_WITH_SAP 1
#define SUBSEGMENT_ALIGNMENT true
#define SUBSEGMENT_STARTS_WITH_SAP 1
#define VIDEO_MIME_TYPE "video/mp4"
#define AUDIO_MIME_TYPE "audio/mp4"
#define AUDIO_LANG "eng"
#define AUDIO_ROLE_SCHEME_ID_URI "urn:mpeg:dash:role:2011"
#define AUDIO_ROLE_VALUE "main"
#define SAR "1:1"
#define AUDIO_CHANNEL_CONFIG_SCHEME_ID_URI "urn:mpeg:dash:23003:3:audio_channel_configuration:2011"

class AdaptationSet;
class VideoAdaptationSet;
class AudioAdaptationSet;
class VideoRepresentation;
class AudioRepresentation;

/*! It is used to manage the MPD File. Use the different setters to fill values and write the file 
    to disk in .mpd format using writeToDisk method */ 

class MpdManager
{
public:
    /**
    * Class constructor
    */
    MpdManager();

    /**
    * Class destructor
    */
    virtual ~MpdManager();

    /**
    * Write to disk the .mpd file with the current data stored in the class. If the .mpd file already exists, it is overwritted
    * @param fileName File name (can be an absolute or relative path)
    */
    void writeToDisk(const char* fileName);

    /**
    * Sets MPD <Location> tag, which represents the MPD file location (must be a URI)
    * @param loc MPD location (e.g. "http://localhost/dash/test.mpd")
    */
    void setLocation(std::string loc);

    /**
    * Sets <MPD> tag attribute 'minimumUpdatePeriod', which represents the MPD file refresh period 
    * @param seconds Value in seconds in seconds
    */
    void setMinimumUpdatePeriod(int seconds);

    /**
    * Sets <MPD> tag attribute 'minBufferTime', which is used by the player to calculate the minimum data to start playing 
    * @param seconds Value in seconds
    */
    void setMinBufferTime(int seconds);

    /**
    * Sets <MPD> tag attribute 'tiemShiftBufferDepth' 
    * @param seconds Value in seconds
    */
    void setTimeShiftBufferDepth(int seconds);

    /**
    * Updates an existing video adaptation set. If it does not exists, it creates a new one. Each adaptation set is
    * represented in the MPD file by the <AdaptationSet> tag. 
    * @param id Adaptation set Id. Must be unique for each one. <AdaptationSet> tag "id" attribute
    * @param timescale Timescale of the media represented by this adaptation set (in ticks per second). <SegmentTemplate> tag "timescale" attribute
    * @param segmentTempl Template for the DASH segments. <SegmentTemplate> tag "media" attribute 
    * @param initTempl Template for the DASH init segments. <SegmentTemplate> tag "initialization" attribute
    */
    void updateVideoAdaptationSet(std::string id, int timescale, std::string segmentTempl, std::string initTempl);

    /**
    * @see updateVideoAdaptationSet
    */
    void updateAudioAdaptationSet(std::string id, int timescale, std::string segmentTempl, std::string initTempl);

    /**
    * Updates an adaptation set timestamp, represented in the MPD file with the tag <S>, child of <SegmentTimeline>. If the
    * number of Timestamps exceeds MAX_SEGMENTS_IN_MPD it replaces the oldest one by the current. If the limit has still not
    * been reached, it adds the current one to the list.
    * @param id Adaptation set Id. Must exist.
    * @param ts Timestamp of the current segment in timescale base.
    * @param duration Duration of the current segment in timescale base.
    * @return true if succeeded and false if not
    */
    bool updateAdaptationSetTimestamp(std::string id, int ts, int duration);

    /**
    * Updates an existing video representation. If it does not exists, it creates a new one. Each representation is
    * represented in the MPD file by the <Representation> tag, child of <AdaptationSet>. 
    * @param adSetId Adaptation set Id. Must exist.
    * @param reprId Representation Id. Must be unique for each one. <Representation> tag "id" attribute.
    * @param codec Video codec
    * @param width Video width in pixels
    * @param height Video height in pixels
    * @param bandwidth Video bandwidth in bits per second
    * @param fps Video framerate
    */
    void updateVideoRepresentation(std::string adSetId, std::string reprId, std::string codec, int width, int height, int bandwidth, int fps);

    /**
    * Updates an existing audio representation. If it does not exists, it creates a new one. Each representation is
    * represented in the MPD file by the <Representation> tag, child of <AdaptationSet>. 
    * @param adSetId Adaptation set Id. Must exist.
    * @param reprId Representation Id. Must be unique for each one. <Representation> tag "id" attribute.
    * @param codec Audio codec
    * @param sampleRate Audio sample rate in Hz
    * @param bandwidth Audio bandwidth in bits per second
    * @param channels Audio channels
    */
    void updateAudioRepresentation(std::string adSetId, std::string reprId, std::string codec, int sampleRate, int bandwidth, int channels);

private:
    bool addAdaptationSet(std::string id, AdaptationSet* adaptationSet);
    AdaptationSet* getAdaptationSet(std::string id);

    std::string minimumUpdatePeriod;
    std::string timeShiftBufferDepth;
    std::string suggestedPresentationDelay;
    std::string minBufferTime;
    std::string location;

    std::map<std::string, AdaptationSet*> adaptationSets;
};

/*! It is used to enapsulate all the data of one <AdaptationSet> tag and its childs. It is the parent class of 
    VideoAdaptationSet and AudioAdaptationSet. */

class AdaptationSet
{
public:
    /**
    * Class constructor
    * @see MpdManager::updateVideoAdaptationSet
    * @see MpdManager::updateAudioAdaptationSet 
    */
    AdaptationSet(int segTimescale, std::string segTempl, std::string initTempl);

    /**
    * Class destructor
    */
    virtual ~AdaptationSet();

    /**
    * It dumps the adaptation set data to the tinyxml2 classes, used to construct the MPD file (which is indeed a XML file).
    * It is a pure virtual method implemented by Video and Audio adaptation sets.
    * @param doc tinyxml2::XMLDocument which represents the whole MPD file
    * @param adaptSet tinyxml2:XMLElement which represents the <AdaptationSet> node.
    */
    virtual void toMpd(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement*& adaptSet) = 0;

    /**
    * @see MpdManager::updateVideoRepresentation 
    */
    virtual void updateVideoRepresentation(std::string id, std::string codec, int width, int height, int bandwidth, int fps){};

    /**
    * @see MpdManager::updateAudioRepresentation 
    */
    virtual void updateAudioRepresentation(std::string id, std::string codec, int sampleRate, int bandwidth, int channels){};

    /**
    * Sets timescale, segment template and init template values
    * @see MpdManager::updateVideoRepresentation 
    * @see MpdManager::updateAudioRepresentation 
    */
    void update(int timescale, std::string segmentTempl, std::string initTempl);

    /**
    * Sets timestamp and duration
    * @see MpdManager::updateAdaptationSetTimestamp 
    */
    void updateTimestamp(int ts, int duration);

protected:
    bool segmentAlignment;
    int startWithSAP;
    bool subsegmentAlignment;
    int subsegmentStartsWithSAP;
    std::string mimeType;
    int timescale;
    std::string segTemplate;
    std::string initTemplate;
    std::deque<std::pair<int,int> > timestamps;
};

/*! It is used to encapsulate all the data of a video AdaptationSet. It adds specific video data to the common data
    of an adaptation set. */

class VideoAdaptationSet : public AdaptationSet 
{
public:
    /**
    * Class constructor
    * @see AdaptationSet::AdaptationSet
    */
    VideoAdaptationSet(int segTimescale, std::string segTempl, std::string initTempl);

    /**
    * Class destructor
    */
    virtual ~VideoAdaptationSet();

    /**
    * @see AdaptationSet::toMpd
    */
    void toMpd(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement*& adaptSet);

    /**
    * @see AdaptationSet::updateVideoRepresentation
    **/
    void updateVideoRepresentation(std::string id, std::string codec, int width, int height, int bandwidth, int fps);

private:
    VideoRepresentation* getRepresentation(std::string id);
    bool addRepresentation(std::string id, VideoRepresentation* repr);
    
    std::map<std::string, VideoRepresentation*> representations;
    int maxWidth;
    int maxHeight;
    int frameRate;
};

/*! It is used to encapsulate all the data of an audio AdaptationSet. It adds specific data of video to the common data
    of an adaptation set. */

class AudioAdaptationSet : public AdaptationSet 
{
public:
    /**
    * Class constructor
    * @see AdaptationSet::AdaptationSet
    */
    AudioAdaptationSet(int segTimescale, std::string segTempl, std::string initTempl);

    /**
    * Class destructor
    */
    virtual ~AudioAdaptationSet();

    /**
    * @see AdaptationSet::toMpd
    */
    void toMpd(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement*& adaptSet);

    /**
    * @see AdaptationSet::updateAudioRepresentation
    **/
    void updateAudioRepresentation(std::string id, std::string codec, int sampleRate, int bandwidth, int channels);

private:
    AudioRepresentation* getRepresentation(std::string id);
    bool addRepresentation(std::string id, AudioRepresentation* repr);
    
    std::map<std::string, AudioRepresentation*> representations;
    std::string lang;
    std::string roleSchemeIdUri;
    std::string roleValue;
};

/*! It is used to encapsulate all the data of a video <Representation> tag.*/

class VideoRepresentation
{
public:
    /**
    * Class constructor
    * @see VideoAdaptationSet::updateVideoRepresentation
    */
    VideoRepresentation(std::string vCodec, int vWidth, int vHeight, int vBandwidth);
    
    /**
    * Class destructor
    */
    virtual ~VideoRepresentation();

    /**
    * Sets codec, width, height and bandwidth
    * @see MpdManager::updateVideoRepresentation 
    */
    void update(std::string vCodec, int vWidth, int vHeight, int vBandwidth);

    /**
    * Get codec as a string
    * @see MpdManager::updateAudioRepresentation 
    */
    std::string getCodec() {return codec;};

    /**
    * Get SAR as string. By default it is 1:1
    */
    std::string getSAR() {return sar;};

    /**
    * Get width in pixels
    */
    int getWidth() {return width;};

    /**
    * Get height in pixels
    */
    int getHeight() {return height;};

    /**
    * Get bandwidth in bits per second
    */
    int getBandwidth() {return bandwidth;};

private:
    std::string codec;
    std::string sar;
    int width;
    int height;
    int bandwidth;
};

/*! It is used to encapsulate all the data of an audio <Representation> tag.*/

class AudioRepresentation
{
public:
    /**
    * Class constructor
    * @see AudioAdaptationSet::updateAudioRepresentation
    */
    AudioRepresentation(std::string aCodec, int aSampleRate, int aBandwidth, int channels);
    
    /**
    * Class destructor
    */
    virtual ~AudioRepresentation();

    /**
    * Sets codec, sample rate, bandwdith and channels
    * @see MpdManager::updateAudioRepresentation 
    */
    void update(std::string aCodec, int aSampleRate, int aBandwidth, int channels);

    std::string getCodec() {return codec;};
    int getSampleRate() {return sampleRate;};
    std::string getAudioChannelConfigSchemeIdUri() {return audioChannelConfigSchemeIdUri;};
    int getBandwidth() {return bandwidth;};
    int getAudioChannelConfigValue() {return audioChannelConfigValue;};

private:
    std::string codec;
    int sampleRate;
    int bandwidth;
    std::string audioChannelConfigSchemeIdUri;
    int audioChannelConfigValue;
};

#endif /* _MPD_MANAGER_HH_ */
