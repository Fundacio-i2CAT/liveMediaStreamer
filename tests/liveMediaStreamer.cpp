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

#include <iostream>
#include "../src/Controller.hh"
#include "../src/Utils.hh"

int main(int argc, char *argv[]) {

    int port;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    Controller* ctrl = Controller::getInstance();

    utils::setLogLevel(ERROR);

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

        ctrl->processRequest();
    }

    ctrl->stopAndCloseSocket();

    return 0; 
}