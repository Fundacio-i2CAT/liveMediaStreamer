/*
 *  Controller.cpp - Controller class for livemediastreamer framework
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
 *			  Martin German <martin.german@i2cat.net>
 */

#include "Controller.hh"
#include "Utils.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

Controller* Controller::ctrlInstance = NULL;
PipelineManager* PipelineManager::pipeMngrInstance = NULL;

Controller::Controller()
{
    ctrlInstance = this;
    pipeMngrInstance = PipelineManager::getInstance();
    inputRootNode = new Jzon::Object();
    parser = new Jzon::Parser(*inputRootNode);
    initializeEventMap();
    runFlag = true;
}

Controller* Controller::getInstance()
{
    if (ctrlInstance != NULL){
        return ctrlInstance;
    }

    return new Controller();
}

void Controller::destroyInstance()
{
    Controller * ctrlInstance = Controller::getInstance();
    if (ctrlInstance != NULL) {
        delete ctrlInstance;
        ctrlInstance = NULL;
    }
}

PipelineManager* Controller::pipelineManager()
{
    return pipeMngrInstance;
}


bool Controller::createSocket(int port)
{
    struct sockaddr_in serv_addr;
    int yes=1;

    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (listeningSocket < 0) {
        utils::errorMsg("Opening socket");
        return false;
    }

    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
        utils::errorMsg("Setting socket options");
        return false;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(listeningSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        utils::errorMsg("On binding");
        return false;
    }

    return true;
}

bool Controller::listenSocket()
{
    socklen_t clilen;
    struct sockaddr cli_addr;
    listen(listeningSocket,5);
    clilen = sizeof(cli_addr);

    connectionSocket = accept(listeningSocket, (struct sockaddr *) &cli_addr, &clilen);

    if (connectionSocket < 0) {
        return false;
    }

    return true;
}

void Controller::stopAndCloseSocket()
{
    close(listeningSocket);
}


bool Controller::readAndParse()
{
    bzero(inBuffer, MSG_BUFFER_MAX_LENGTH);
    inputRootNode->Clear();

    if (read(connectionSocket, inBuffer, MSG_BUFFER_MAX_LENGTH - 1) < 0) {
        utils::errorMsg("Reading from socket");
        return false;
    }

    parser->SetJson(inBuffer);

    if (!parser->Parse()) {
        utils::errorMsg("Error parsing JSON");
        return false;
    }

    return true;
}

void Controller::processRequest()
{
    Jzon::Object outputNode;

    if (!inputRootNode->Has("events") || !inputRootNode->Get("events").IsArray()) {
        utils::warningMsg("Invalid JSON, missing 'events' tag");
        outputNode.Add("error", "Invalid JSON, missing 'events' tag");
        sendAndClose(outputNode, connectionSocket);
        return;
    }

    const Jzon::Array &events = inputRootNode->Get("events").AsArray();

    for (Jzon::Array::const_iterator it = events.begin(); it != events.end(); ++it) {
        if ((*it).Has("filterId")) {
            processFilterEvent(*it, outputNode);
        } else {
            processInternalEvent(*it, outputNode);
        }
    }

    sendAndClose(outputNode, connectionSocket);
}

void Controller::processFilterEvent(Jzon::Object event, Jzon::Object &outputNode)
{
    int filterId;
    int delay = -1;
    BaseFilter *filter = NULL;

    if (!event.Has("action") || !event.Has("params")) {
        outputNode.Add("error", "Error processing filter event. Invalid JSON format...");
        return ;
    }

    filterId = event.Get("filterId").ToInt();
    filter = pipeMngrInstance->getFilter(filterId);

    if (!filter) {
        outputNode.Add("error", "Error while processing event. There is no filter with this ID...");
        return;
    }

    if (event.Has("delay")) {
        delay = event.Get("delay").ToInt();
    }

    Event e(event, std::chrono::system_clock::now(), delay);
    filter->pushEvent(e);
    outputNode.Add("error", Jzon::null);
}

void Controller::processInternalEvent(Jzon::Object event, Jzon::Object &outputNode)
{
    if (!event.Has("action") || !event.Has("params")) {
        outputNode.Add("error", "Error processing internal event. Invalid JSON format...");
        return;
    }

    std::string action = event.Get("action").ToString();
    Jzon::Object params = event.Get("params");

    if (eventMap.count(action) <= 0) {
        outputNode.Add("error", "Error processing internal event. Invalid action...");
        return;
    }

    eventMap[action](&params, outputNode);
}

void Controller::initializeEventMap()
{
    eventMap["getState"] = std::bind(&PipelineManager::getStateEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["createPath"] = std::bind(&PipelineManager::createPathEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["removePath"] = std::bind(&PipelineManager::removePathEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["createFilter"] = std::bind(&PipelineManager::createFilterEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["addSlavesToFilter"] = std::bind(&PipelineManager::addSlavesToFilterEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);
    eventMap["stop"] = std::bind(&PipelineManager::stopEvent, pipeMngrInstance,
                                            std::placeholders::_1, std::placeholders::_2);

}

void Controller::sendAndClose(Jzon::Object outputNode, int socket)
{
    Jzon::Writer writer(outputNode, Jzon::NoFormat);
    writer.Write();
    std::string result = writer.GetResult();
    const char* res = result.c_str();
    int ret = write(socket, res, result.size());

    if (ret < 0) {
        utils::errorMsg("Error writting socket");
    }

    if (socket >= 0){
        close(socket);
    }
}
