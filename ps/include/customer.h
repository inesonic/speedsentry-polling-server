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
* This header defines the \ref Customer class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QHash>
#include <QMutex>

#include <cstdint>

#include <monitor.h>
#include <host_scheme.h>

class HttpServiceThread;

/**
 * Class that tracks all the host/scheme instances for a customer.
 */
class Customer:public QObject {
    Q_OBJECT

    friend class HostScheme;
    friend class HttpServiceThread;

    public:
        /**
         * Value used to represent a customer ID.
         */
        typedef std::uint32_t CustomerId;

        /**
         * Hash table of host/schemes under this customer.
         */
        typedef QHash<HostScheme::HostSchemeId, HostScheme*> HostSchemesByHostSchemeId;

        /**
         * Constructor.
         *
         * \param[in] customerId                    The ID used to identify the customer.
         *
         * \param[in] supportsPingTesting           If true, this customer subscription includes ping based testing of
         *                                          servers.
         *
         * \param[in] supportsSslExpirationChecking If true, this customer subscription includes SSL expiration
         *                                          checking.
         *
         * \param[in] supportsLatencyMeasurements   If true, latency measurements should be collected for this
         *                                          customer.
         *
         * \pmara[in] supportsMultiRegionTesting    If true, latency measurements are done across regions.
         *
         * \param[in] pollingInterval               The polling interval to use for this customer, in seconds.
         *
         * \param[in] serviceThread                 Pointer to the service thread owning this customer instance.
         */
        Customer(
            CustomerId         customerId,
            bool               supportsPingTesting,
            bool               supportsSslExpirationChecking,
            bool               supportsLatencyMeasurements,
            bool               supportsMultiRegionTesting,
            unsigned           pollingInterval,
            HttpServiceThread* serviceThread = nullptr
        );

        ~Customer();

        /**
         * Method you can use to add a host scheme under this customer.   Host/schemes are tracked by host/scheme ID
         * If a host/scheme with the same host/scheme ID is already registered with this host/scheme, the
         * existing host scheme will be deleted and replaced.
         *
         * The host scheme will also be updated to point to this customer.
         *
         * \param[in] hostScheme The host/scheme to be added.
         */
        void addHostScheme(HostScheme* hostScheme);

        /**
         * Method you can use to remove a host/scheme from this customer.  The host scheme and all associated monitors
         * will be deleted.
         *
         * Note that you should always use this method rather than deleting the host/scheme directly.
         *
         * \param[in] hostSchemeId The ID of the host/scheme to be removed.
         *
         * \return Returns true on success.  Returns false if the monitor is not tied to this host scheme instance.
         */
        bool removeHostScheme(HostScheme::HostSchemeId hostSchemeId);

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
         * Method you can use to obtain all host/scheme instances under this customer.
         *
         * \return Returns a list of host/scheme instances.
         */
        QList<HostScheme*> hostSchemes() const;

        /**
         * Method you can use to obtain all monitors under this customer.
         *
         * \return Returns a list of monitor instances.
         */
        QList<Monitor*> monitors() const;

        /**
         * Method you can use to obtain the customer ID.
         *
         * \return Returns the customer ID of this customer.
         */
        CustomerId customerId() const;

        /**
         * Method that indicates if this customer's subscription includes ping based testing.
         *
         * \return Returns true if ping testing is enabled.  Returns false if ping based testing is not enabled.
         */
        bool supportsPingTesting() const;

        /**
         * Method you can use to indicate if this customer's subscription includes ping testing.
         *
         * \param[in] nowSupported If true, then ping based testing should be included.  If false, ping based testing
         *                         is not included.
         */
        void setSupportsPingTesting(bool nowSupported);

        /**
         * Method that indicates if this customer's subscription includes SSL expiration checking.
         *
         * \return Returns true if SSL expiration checking is supported.  Returns false if SSL expiration checking is
         *         not supported.
         */
        bool supportsSslExpirationChecking() const;

        /**
         * Method you can use to indicate if this customer's subscription includes SSL expiration checking.
         *
         * \param[in] nowSupported If true, then SSL expiration checking is to be supported.  If false, then SSL
         *                         expiration checking is not supported.
         */
        void setSupportsSslExpirationChecking(bool nowSupported);

        /**
         * Method that indicates if this customer's subscription includes latency measurements.
         *
         * \return Returns true if latency measurements should be collected.  Returns false if latency measurements
         *         are not to be collected.
         */
        bool supportsLatencyMeasurements() const;

        /**
         * Method you can use to indicate if this customer's subscription includes latency measurements.
         *
         * \param[in] nowSupported If true, then latency measurements should be collected.  If false, then latency
         *                         measurements should not be collected.
         */
        void setSupportsLatencyMeasurements(bool nowSupported);

        /**
         * Method that indicates if this customer should be tracked in multiple geographic regions.  We need this
         * information here so that we can take extra measures to guarantee consistent timing across regions.
         *
         * \return Returns true if this customer should be tracked/measured across multiple geographic regions.
         *         Returns false if this customer should be tracked in a single geographic region.
         */
        bool supportsMultiRegionTesting() const;

        /**
         * Method you can use to indicate if this customer should be tracked in multiple geographic regions.  We need
         * this information here so that we can take extra measures to guarantee consistent timing across regions.
         *
         * \param[in] nowSupported If true, then testing for this customer is being performed across multiple regions.
         */
        void setSupportsMultiRegionTesting(bool nowSupported);

        /**
         * Method you can use to determine the current polling interval for this customer.
         *
         * \return Returns the polling interval for this customer, in seconds.
         */
        unsigned pollingInterval() const;

        /**
         * Method you can use to change the polling interval for this customer.
         *
         * \param[in] newPollingInterval The new polling interval to use for this customer, in seconds.
         */
        void setPollingInterval(unsigned newPollingInterval);

        /**
         * Method you can use to determine if this monitor is paused.
         *
         * \return Returns true if this monitor is paused.
         */
        bool paused() const;

        /**
         * Method you can use to set this monitor as paused or unpaused.
         *
         * \param[in] nowPaused If true, the monitor will pause.  If false, the monitor will resume.
         */
        void setPaused(bool nowPaused = true);

        /**
         * Method that returns the number of host/scheme instances under this customer.
         *
         * \return Returns the number of host/scheme instances under this customer.
         */
        unsigned numberHostSchemes() const;

        /**
         * Method that returns the number of monitors under this customer.
         *
         * \return Returns the number of monitors under this customer.
         */
        unsigned numberMonitors() const;

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
         * Method that is called by the customer to propagate information about host schemes and monitors under this
         * customer.
         *
         * \param[in] serviceThread The service thread the host schemes should be reported to.
         *
         * \param[in] adding        If true, we're being added.  If false, we're being removed.
         */
        void reportExistingHostSchemesAndMonitors(HttpServiceThread* customer, bool adding);

        /**
         * The service thread that is processing this customer.
         */
        HttpServiceThread* currentServiceThread;

        /**
         * The customer ID of this customer.
         */
        CustomerId currentCustomerId;

        /**
         * Holds true if this customer's subscription supports ping testing.
         */
        bool currentSupportsPingTesting;

        /**
         * Holds true if this customer's subscription supports SSL expiration testing.
         */
        bool currentSupportsSslExpirationChecking;

        /**
         * Holds true if this customer's subscription includes latency measurements.
         */
        bool currentSupportsLatencyMeasurements;

        /**
         * Holds true if this customer should be tracked/measured across multiple geographic regions.
         */
        bool currentSupportsMultiRegionTesting;

        /**
         * Holds the polling interval to use for this customer.
         */
        unsigned currentPollingInterval;

        /**
         * Mutex used to protect our host/scheme hash table.
         */
        mutable QMutex hostSchemeMutex;

        /**
         * Hash table of host/schemes by host/scheme ID.
         */
        HostSchemesByHostSchemeId hostSchemesByHostSchemeId;

        /**
         * Mutex used to protect our monitors hash table.
         */
        mutable QMutex monitorMutex;

        /**
         * Hash table of monitors under this customer.
         */
        HostScheme::MonitorsByMonitorId monitorsByMonitorId;

        /**
         * Flag indicating if this monitor is paused.
         */
        bool currentlyPaused;
};

#endif
