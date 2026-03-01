/**
 * @file WebServer.cpp
 * @brief Implementation of the WebServer class.
 */

#include "WebServer.h"

WebServer::WebServer(int id, int cooldownCycles)
    : id_(id), idle_(true), currentRequest_(), completedCount_(0),
      justCompleted_(false), justAssigned_(false),
      cooldownCycles_(cooldownCycles), cooldownRemaining_(0)
{
}

bool WebServer::isIdle() const
{
    return idle_;
}

bool WebServer::isAvailable() const
{
    return idle_ && cooldownRemaining_ == 0;
}

void WebServer::assignRequest(const Request &request)
{
    currentRequest_ = request;
    idle_ = false;
    justCompleted_ = false;
    justAssigned_ = true;
}

void WebServer::tick()
{
    if (idle_)
    {
        justCompleted_ = false;
        if (cooldownRemaining_ > 0)
        {
            --cooldownRemaining_;
        }
        return;
    }

    // Skip the first tick on the cycle a request is assigned so that a
    // request with duration N completes exactly N cycles after it starts.
    if (justAssigned_)
    {
        justAssigned_ = false;
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
        cooldownRemaining_ = cooldownCycles_;
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

int WebServer::getCooldownRemaining() const
{
    return cooldownRemaining_;
}

