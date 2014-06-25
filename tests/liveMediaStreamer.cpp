/*
 *  MIXER - A real-time video mixing application
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin MIXER.
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 *            Ignacio Contreras <ignacio.contreras@i2cat.net>
 */     


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <string>
#include <iostream>
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"
#include "../src/Utils.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"

void createMixerEncoderTxPath()
{
    PipelineManager *pipeMngr = Controller::getInstance()->pipelineManager();
    
    AudioMixer *mixer = new AudioMixer(4);
    int audioMixerID = rand();

    if(!pipeMngr->addFilter(audioMixerID, mixer)) {
        std::cerr << "Error adding mixer to the pipeline" << std::endl;
    }

    Path *path = new AudioEncoderPath(audioMixerID, pipeMngr->getFilter(audioMixerID)->generateWriterID());
    dynamic_cast<AudioEncoderLibav*>(pipeMngr->getFilter(path->getFilters().front()))->configure(OPUS);

    path->setDestinationFilter(pipeMngr->getTransmitterID(), pipeMngr->getTransmitter()->generateReaderID());

    if (!pipeMngr->connectPath(path)) {
        exit(1);
    }

    int encoderPathID = rand();

    if (!pipeMngr->addPath(encoderPathID, path)) {
        exit(1);
    }

    Worker* audioMixerWorker = new BestEffort();
    if(!pipeMngr->addWorker(audioMixerID, audioMixerWorker)) {
        std::cerr << "Error adding mixer worker" << std::endl;
        exit(1);
    }

    std::vector<int> readers;
    std::string sessionId = utils::randomIdGenerator(ID_LENGTH);

    readers.push_back(path->getDstReaderID());
        
    if(!pipeMngr->getTransmitter()->addSession(sessionId, readers)) {
        std::cerr << "Error adding session to transsmiter" << std::endl;
        exit(1);
    }

    pipeMngr->getTransmitter()->publishSession(sessionId);

    pipeMngr->startWorkers();

}

int main(int argc, char *argv[]) {

    int sockfd, newsockfd, port, n;
    char buffer[2048];
    struct timeval in_time;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    Controller* ctrl = Controller::getInstance();
    ctrl->pipelineManager()->getReceiver()->setCallback(callbacks::connectToMixerCallback);

    createMixerEncoderTxPath();

    port = atoi(argv[1]);
    if (!ctrl->createSocket(port)) {
        exit(1);
    }

    while (ctrl->run()) {
        if (!ctrl->listenSocket()) {
            continue;
        }

        if (!ctrl->readAndParse()) {
            //TDODO: error msg
            continue;
        }

        ctrl->processEvent();
    }

    close(sockfd);
    return 0; 
}