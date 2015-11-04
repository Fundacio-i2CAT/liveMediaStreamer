/*
 *  Controller.hh - Controller class for livemediastreamer framework
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
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
 *            David Cassany <david.cassany@i2cat.net>
 */

#ifndef _CONTROLLER_HH
#define _CONTROLLER_HH

#include <vector>
#include <map>
#include <functional>

#include "PipelineManager.hh"

#define MSG_BUFFER_MAX_LENGTH 4096*4
#define TIMEOUT 100000 //usec

/*! Controller class is a singleton class defines the control protocol
    by events and through sockets
*/
class Controller {
public:
    /**
    * Gets the Controller object pointer of the instance. Creates a new
    * instance for first time or returns the same instance if it already exists.
    * @return Controller instance pointer
    */
    static Controller* getInstance();

    /**
    * If PipelineManager instance exists it is destroyed.
    */
    static void destroyInstance();

    /**
    * Returns the pipelinemanager object of the controller
    */
    PipelineManager* pipelineManager();

    /**
    * Creates a TCP socket from incoming port
    * @param port to set the TCP socket
    * @return true if succes, otherwise returns false
    */
    bool createSocket(int port);

    /**
    * Listens incoming data from socket
    * @return true if succes, otherwise returns false
    */
    bool listenSocket();

    /**
    * Stops and closes socket
    */
    void stopAndCloseSocket();

    /**
    * Reads and parses incoming data from socket
    * @return true if succes, otherwise returns false
    */
    bool readAndParse();

    /**
    * Returns running status
    * @return true if controller object is properly created, otherwise return false
    */
    bool run() {return runFlag;};

    /**
    * Processes the incoming requests from socket
    */
    void processRequest();

protected:
    void initializeEventMap();

private:
    Controller();
    bool processEvent(Jzon::Object event);
    void processFilterEvent(Jzon::Object event, Jzon::Object &outputNode);
    void processInternalEvent(Jzon::Object event, Jzon::Object &outputNode);
    void sendAndClose(Jzon::Object outputNode, int socket);

    int listeningSocket, connectionSocket;
    char inBuffer[MSG_BUFFER_MAX_LENGTH];
    Jzon::Object* inputRootNode;
    Jzon::Parser* parser;
    std::map<std::string, std::function<void(Jzon::Node* params, Jzon::Object &outputNode)> > eventMap;
    bool runFlag;

    static Controller* ctrlInstance;
    PipelineManager* pipeMngrInstance;

    fd_set readfds;
};

#endif
