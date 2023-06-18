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
* This header defines the \ref HttpServiceThread class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef HTTP_SERVICE_THREAD_H
#define HTTP_SERVICE_THREAD_H

#include <QThread>
#include <QPointer>
#include <QHash>
#include <QMultiMap>

#include <cstdint>

#include "customer.h"
#include "loading_data.h"
#include "service_thread.h"

class QNetworkAccessManager;

class HostSchemeTimer;
class DataAggregator;

/**
 * Class that manages an independent monitor service thread that performs HTTP status checks.
 */
class HttpServiceThread:public ServiceThread {
    Q_OBJECT

    friend class Customer;

    public:
        /**
         * Hash table of host/schemes under this customer.
         */
        typedef QHash<Customer::CustomerId, Customer*> CustomersByCustomerId;

        /**
         * Constructor.
         *
         * \param[in] dataAggregator The data aggregator to be used by this thread.
         *
         * \param[in] parent         Pointer to the parent object.
         */
        HttpServiceThread(DataAggregator* dataAggregator, QObject* parent = nullptr);

        ~HttpServiceThread() override;

        /**
         * Method you can use to obtain a service metric for this thread.  Returned value is in host/schemes or sites
         * serviced per second.
         *
         * \return Returns the current service metric.
         */
        float hostSchemesPerSecond() const;

        /**
         * Method you can use to obtain detailed loading data.
         *
         * \return Returns a map of loading data instances.  The key is the polling interval, in seconds.  Negative
         *         values indicate single regions.  Positive values indicate multi-region polling.
         */
        QMultiMap<int, LoadingData> loadingData() const;

        /**
         * Method you can use to obtain the data aggregator being used by this thread.
         *
         * \return Returns the data aggregator being used by this thread.
         */
        inline DataAggregator* dataAggregator() const {
            return currentDataAggregator;
        }

        /**
         * Method you can use to add a customer to this service thread.
         *
         * \param[in] customer The customer instance to be added.
         */
        void addCustomer(Customer* customer);

        /**
         * Method you can use to remove a customer from this service thread.
         *
         * Note that you should always use this method rather than deleting the customer directly.
         *
         * \param[in] customerId The zero based ID of the customer to be removed.
         *
         * \return Returns true if the customer is managed by this thread.  Returns false if the customer is not
         *         managed by this thread.
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

    public slots:
        /**
         * Slot you can trigger to request that a given host/scheme be checked immediately
         *
         * \param[in] hostScheme The host/scheme instance to be checked.
         */
        void checkNow(QPointer<HostScheme> hostScheme);

        /**
         * Slot you can use to update the region settings.  This method will also cause this thread to go active.
         *
         * \param[in] regionIndex   The zero based region index for the region we're in.
         *
         * \param[in] numberRegions The number of regions we're in.
         */
        void updateRegionData(unsigned regionIndex, unsigned numberRegions);

        /**
         * Slot you can trigger to command this thread to go inactive.
         */
        void goInactive() override;

        /**
         * Slot you can trigger to command this thread to go active.
         */
        void goActive() override;

    protected:
        /**
         * Method you can use to run this thread.  The method simply starts the thread's exec loop.
         */
        void run() override;

    private:
        /**
         * Method that is called by the host/scheme when a monitor has been added.
         *
         * \param[in] monitor The newly added monitor.
         */
        void monitorAdded(Monitor* monitor);

        /**
         * Method that is called by the host/scheme when a monitor has been removed.
         *
         * \param[in] monitor The monitor that is being removed.
         */
        void monitorAboutToBeRemoved(Monitor* monitor);

        /**
         * Method that is called by the host/scheme when it is newly created.
         *
         * \param[in] hostScheme The newly added monitor.
         */
        void hostSchemeAdded(HostScheme* hostScheme);

        /**
         * Method that is called by the host/scheme when it is about to be deleted.
         *
         * \param[in] hostScheme The host/scheme that is being removed.
         */
        void hostSchemeAboutToBeRemoved(HostScheme* hostScheme);

        /**
         * Method that is called by the customer when it is newly created.
         *
         * \param[in] customer The newly added monitor.
         */
        void customerAdded(Customer* customer);

        /**
         * Method that is called by the customer when it is about to be deleted.
         *
         * \param[in] customer The customer that is being removed.
         */
        void customerAboutToBeRemoved(Customer* customer);

        /**
         * Method that updates the monitor service metrics.
         */
        void updateServiceMetrics();

        /**
         * The current monitor service metric in host/schemes per second.
         */
        float currentHostSchemesPerSecond;

        /**
         * Object defined within the thread context.
         */
        QObject* currentThreadObject;

        /**
         * The data aggregator to be used by this thread.
         */
        DataAggregator* currentDataAggregator;

        /**
         * Mutex used to protect our customer hash table.
         */
        mutable QMutex customerMutex;

        /**
         * Hash table of customers by customer ID.
         */
        CustomersByCustomerId customersByCustomerId;

        /**
         * Mutex used to protect our host/scheme hash table.
         */
        mutable QMutex hostSchemeMutex;

        /**
         * Map of host/scheme timer by polling interval.  Negative values are used to indicate single region monitors,
         * positive values are used to indicate multi-region monitors.
         */
        QMap<int, HostSchemeTimer*> hostSchemeTimers;

        /**
         * Mutex used to protect our monitors hash table.
         */
        mutable QMutex monitorMutex;

        /**
         * Hash table of monitors under this host/scheme.
         */
        HostScheme::MonitorsByMonitorId monitorsByMonitorId;

        /**
         * The network access manager for this thread.
         */
        QNetworkAccessManager* networkAccessManager;

        /**
         * The current region index we're operating in.
         */
        unsigned currentRegionIndex;

        /**
         * The current number of known regions.
         */
        unsigned currentNumberRegions;

        /**
         * Flag indicating if we're active.
         */
        bool currentActive;
};

#endif
