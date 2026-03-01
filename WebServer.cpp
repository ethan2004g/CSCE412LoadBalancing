/**
 * @file WebServer.cpp
 * @brief Implementation of the WebServer class.
 */

#include "WebServer.h"

WebServer::WebServer(int id)
    : id_(id), idle_(true), currentRequest_(), completedCount_(0), justCompleted_(false)
{
}

bool WebServer::isIdle() const
{
    return idle_;
}

void WebServer::assignRequest(const Request &request)
{
    currentRequest_ = request;
    idle_ = false;
    justCompleted_ = false;
}

void WebServer::tick()
{
    if (idle_)
    {
        justCompleted_ = false;
        return;
    }

    if (currentRequest_.timeRemaining > 0)
    {
        --currentRequest_.timeRemaining;
    }

    if (currentRequest_.timeRemaining <= 0)
    {
        idle_ = true;
        ++completedCount_;
        justCompleted_ = true;
    }
    else
    {
        justCompleted_ = false;
    }
}

int WebServer::getCompletedCount() const
{
    return completedCount_;
}

int WebServer::getId() const
{
    return id_;
}

bool WebServer::hasJustCompleted() const
{
    return justCompleted_;
}

void WebServer::clearJustCompleted()
{
    justCompleted_ = false;
}

const Request &WebServer::getCurrentRequest() const
{
    return currentRequest_;
}

