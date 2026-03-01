/**
 * @file WebServer.h
 * @brief Definition of the WebServer class used by the load balancer.
 */

#pragma once

#include "Request.h"

/**
 * @brief Represents a single web server instance that can process requests.
 *
 * A WebServer either has an active request or is idle. It can be used by the
 * LoadBalancer to process queued requests.
 */
class WebServer
{
public:
    /**
     * @brief Construct a new WebServer.
     *
     * @param id Unique identifier for this server.
     */
    explicit WebServer(int id);

    /**
     * @brief Check whether this server is currently idle.
     *
     * @return true if there is no active request; false otherwise.
     */
    bool isIdle() const;

    /**
     * @brief Assign a new request to this server.
     *
     * @param request The request to process.
     */
    void assignRequest(const Request &request);

    /**
     * @brief Advance the server by one clock cycle.
     *
     * Decrements the remaining time on the current request, if any, and marks
     * the request as completed when its time reaches zero.
     */
    void tick();

    /**
     * @brief Get the number of requests this server has completed.
     *
     * @return Number of completed requests.
     */
    int getCompletedCount() const;

    /**
     * @brief Get the server's identifier.
     *
     * @return The server ID.
     */
    int getId() const;

    /**
     * @brief Determine whether the current request (if any) has completed.
     *
     * @return true if the server just finished a request in the last tick; false otherwise.
     */
    bool hasJustCompleted() const;

    /**
     * @brief Clear the "just completed" flag after it has been recorded.
     */
    void clearJustCompleted();

    /**
     * @brief Get the request currently assigned to (or last completed by) this server.
     *
     * @return Const reference to the current request.
     */
    const Request &getCurrentRequest() const;

private:
    int id_;
    bool idle_;
    Request currentRequest_;
    int completedCount_;
    bool justCompleted_;
    bool justAssigned_;
};

