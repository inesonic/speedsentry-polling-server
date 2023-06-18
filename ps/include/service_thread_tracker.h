/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 - 2023 Inesonic, LLC.
*
* GNU Public License, Version 3:
*   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program.  If not, see
*   <https://www.gnu.org/licenses/>.
********************************************************************************************************************//**
* \file
*
* This header defines the \ref ServiceThreadTracker class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef SERVICE_THREAD_TRACKER_H
#define SERVICE_THREAD_TRACKER_H

#include <QObject>
#include <QHash>
#include <QMultiMap>
#include <QList>

#include <cstdint>

#include <customer.h>
#include <loading_data.h>

class DataAggregator;
class HttpServiceThread;
class PingServiceThread;

/**
 * Class that can be used to track and manage a collection of service threads.
 */
class ServiceThreadTracker:public QObject {
    Q_OBJECT

    public:
        /**
         * Enumeration of supported server status codes.  Note this structure is also defined in the db_controller
         * project with values that must match.
         */
        enum class Status : std::uint8_t {
            /**
             * Indicates all or unknown status.  This value will never be used by a server and is used to either
             * indicate we want all possible status or that the server's status has not yet been determined.
             */
            ALL_UNKNOWN = 0,

            /**
             * Indicates the server is active.
             */
            ACTIVE = 1,

            /**
             * Indicates the server is inactive.
             */
            INACTIVE = 2,

            /**
             * Indicates the server is defunct and no longer exists.
             */
            DEFUNCT = 3,

            /**
             * Indicates the number of server status values.
             */
            NUMBER_VALUES
        };

        /**
         * Constructor.
         *
         * \param[in] dataAggregator       The data aggregator used to report latency and events.
         *
         * \param[in] maximumNumberThreads The maximum number of threads we should allow.  A value of 0 will cause the
         *                                 number of threads to be based on the number of logical cores.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        ServiceThreadTracker(
            DataAggregator* dataAggregator,
            unsigned        maximumNumberThreads = 0,
            QObject*        parent = nullptr
        );

        ~ServiceThreadTracker() override;

        /**
         * Method you can use to connect to the ping server.
         *
         * \param[in] socketName The name of the ping server socket.
         */
        void connectToPinger(const QString& socketName);

        /**
         * Method you can use to obtain detailed loading data.
         *
         * \return Returns a map of loading data instances.  The key is the polling interval, in seconds.  Negative
         *         values indicate single regions.  Positive values indicate multi-region polling.
         */
        QMultiMap<int, LoadingData> loadingData() const;

        /**
         * Method you can use to obtain a monitor service metric for this server.  Returned value is in monitors per
         * second.
         *
         * \return Returns the current monitor service metric.
         */
        float monitorsPerSecond() const;

        /**
         * Method you can use to determine the current server status.
         *
         * \return Returns the current status.  Values will either be \ref Status::ACTIVE or \ref Status::INACTIVE.
         */
        Status status() const;

        /**
         * Method you can use to add a customer to a service thread.  The customer will be added to the most lightly
         * loaded threads.
         *
         * \param[in] customer The customer instance to be added.
         */
        void addCustomer(Customer* customer);

        /**
         * Method you can use to remove a customer from a service thread.
         *
         * \param[in] customerId The zero based ID of the customer to be removed.
         *
         * \return Returns true if the customer is managed by one of the threads.  Returns false if the customer is not
         *         managed by one of the threads.
         */
        bool removeCustomer(Customer::CustomerId customerId);

        /**
         * Method you can use to obtain a pointer to a customer by customer ID.
         *
         * \param[in] customerId The ID of the customer of interest.
         *
         * \return Returns the desired customer.
         */
        Customer* getCustomer(Customer::CustomerId customerId) const;

        /**
         * Method you can use to obtain a pointer to a host/scheme by host/scheme ID.
         *
         * \param[in] hostSchemeId The ID of the host/scheme of interest.
         *
         * \return Returns the desired host/scheme.
         */
        HostScheme* getHostScheme(HostScheme::HostSchemeId hostSchemeId) const;

        /**
         * Method you can use to obtain a pointer to a monitor by monitor ID.
         *
         * \param[in] monitorId The ID of the monitor of interest.
         *
         * \return Returns the desired monitor.
         */
        Monitor* getMonitor(Monitor::MonitorId monitorId) const;

        /**
         * Method you can use to determine if a customer is currently paused.
         *
         * \param[in] customerId The ID of the customer of interest.
         *
         * \return Returns true if the customer is paused.  Returns false if the customer is not paused.
         */
        bool paused(Customer::CustomerId customerId) const;

        /**
         * Method you can use to pause a customer.
         *
         * \param[in] customerId The ID of the customer of interest.
         *
         * \param[in] nowPaused  If true, the customer's polling should be paused.  If false, the customer's polling
         *                       should be resumed.
         */
        void setPaused(Customer::CustomerId customerId, bool nowPaused);

        /**
         * Method that converts the current status to a string.
         *
         * \param[in] status The status to be converted.
         *
         * \return Returns the status as a string.
         */
        static QString toString(Status status);

    public slots:
        /**
         * Slot you can use to update the region settings.  This method will also cause all service threads to go
         * active.
         *
         * \param[in] regionIndex   The zero based region index for the region we're in.
         *
         * \param[in] numberRegions The number of regions we're in.
         */
        void updateRegionData(unsigned regionIndex, unsigned numberRegions);

        /**
         * Slot you can trigger to command the service threads to go active.
         *
         * \param[in] nowActive If true, the services should go active.  If false, the services should go inactive.
         */
        void goActive(bool nowActive = true);

        /**
         * Slot you can trigger to command the service threads to go inactive.
         *
         * \param[in] nowInactive If true, the services should go inactive.  If false, the services should go active.
         */
        void goInactive(bool nowInactive = true);

    private:
        /**
         * The list of HTTP service threads.
         */
        QList<HttpServiceThread*> httpServiceThreads;

        /**
         * The data aggregator.
         */
        DataAggregator* currentDataAggregator;

        /**
         * The ping service thread.
         */
        PingServiceThread* pingServiceThread;

        /**
         * The current server status.
         */
        Status currentStatus;
};

#endif
