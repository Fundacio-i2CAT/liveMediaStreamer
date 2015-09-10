# LiveMediaStreamer Container
FROM ubuntu:14.04
MAINTAINER Gerard CL <gerardcl@gmail.com>

RUN apt-get update && apt-get -y upgrade

RUN apt-get -y install git cmake autoconf automake build-essential libass-dev \
libtheora-dev libtool libvorbis-dev pkg-config zlib1g-dev libcppunit-dev yasm \ 
libx264-dev  libmp3lame-dev  libopus-dev libvpx-dev liblog4cplus-dev \ 
libtinyxml2-dev opencv-data libopencv-dev mercurial cmake-curses-gui vim \
libcurl3 wget curl 

RUN adduser --disabled-password --gecos '' lms && adduser lms sudo \
	&& echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER lms

RUN hg clone https://bitbucket.org/multicoreware/x265 /home/lms/x265 \
	&& cd /home/lms/x265 && cmake -G "Unix Makefiles" ./source \
	&& make -j4 && sudo make install && sudo ldconfig

RUN git clone https://github.com/mstorsjo/fdk-aac.git/ /home/lms/fdk-aac \
	&& cd /home/lms/fdk-aac && libtoolize && ./autogen.sh \
	&& ./configure && make -j4 && sudo make install && sudo ldconfig

RUN cd /home/lms && wget http://ffmpeg.org/releases/ffmpeg-2.7.tar.bz2 \
	&& tar xjvf ffmpeg-2.7.tar.bz2 && cd ffmpeg-2.7 \
	&& ./configure --enable-gpl --enable-libass --enable-libtheora \
	--enable-libvorbis --enable-libx264 --enable-nonfree --enable-shared \
	--enable-libopus --enable-libmp3lame --enable-libvpx --enable-libfdk_aac \
	--enable-libx265 && make -j4 && sudo make install && sudo ldconfig

RUN cd /home/lms && wget \
	http://www.live555.com/liveMedia/public/live555-latest.tar.gz \
	&& tar xaf live555-latest.tar.gz && cd live \
	&& ./genMakefiles linux-with-shared-libraries && make -j4 \
	&& sudo make install && sudo ldconfig

RUN git clone https://github.com/ua-i2cat/livemediastreamer.git \
	/home/lms/livemediastreamer && cd /home/lms/livemediastreamer \
	&& git checkout development && ./autogen.sh \
	&& make -j4 && sudo make install && sudo ldconfig

EXPOSE 5000-5017/udp
EXPOSE 8554-8564
EXPOSE 7777

CMD ["/usr/local/bin/livemediastreamer","7777"]
