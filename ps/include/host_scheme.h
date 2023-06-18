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
* This header defines the \ref HostScheme class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef HOST_SCHEME_H
#define HOST_SCHEME_H

#include <QString>
#include <QUrl>
#include <QHash>
#include <QMutex>
#include <QSet>

#include <cstdint>

#include <monitor.h>

class QNetworkAccessManager;
class Customer;

/**
 * Class that manages a host/scheme, including background ping based polling and timing of monitor checks.
 */
class HostScheme:public QObject {
    Q_OBJECT

    friend class Monitor;
    friend class Customer;

    public:
        /**
         * Value used to represent a host/scheme ID.
         */
        typedef std::uint32_t HostSchemeId;

        /**
         * Value used to represent a customer ID.
         */
        typedef std::uint32_t CustomerId;

        /**
         * Enumeration of supported event types.
         */
        enum class EventType : std::uint8_t {
            /**
             * Indicates an invalid event type.
             */
            INVALID,

            /**
             * Indicates that the monitor is now working.
             */
            WORKING,

            /**
             * Indicates no response from the requested endpoint.
             */
            NO_RESPONSE,

            /**
             * Indicates that the site content changed state.
             */
            CONTENT_CHANGED,

            /**
             * Indicates the keyword check for the endpoint failed.
             */
            KEYWORDS,

            /**
             * Indicates an SSL certificate is due to expire.
             */
            SSL_CERTIFICATE
        };

        /**
         * Hash table of monitors under this host/scheme.
         */
        typedef QHash<Monitor::MonitorId, Monitor*> MonitorsByMonitorId;

        /**
         * Value indicating an invalid or unknown SSL expiration timestamp.
         */
        static constexpr unsigned long long invalidSslExpirationTimestamp = 0;

        /**
         * Constructor.
         *
         * \param[in] hostSchemeId The ID used to identify this host and scheme.
         *
         * \param[in] url          The server URL.
         *
         * \param[in] customer     The customer owning this host/scheme.
         */
        HostScheme(HostSchemeId hostSchemeId, const QUrl& url, Customer* customer = nullptr);

        ~HostScheme() override;

        /**
         * Method you can use to determine the network access manager being used by monitors under this host/scheme.
         *
         * \return Returns the network access manager being used by this monitor.
         */
        QNetworkAccessManager* networkAccessManager() const;

        /**
         * Method you can use to set the network access manager to be used by monitors under this host/scheme.
         *
         * \param[in] newNetworkAccessManager The new network access manager to be used by this monitor.
         */
        void setNetworkAccessManager(QNetworkAccessManager* newNetworkAccessManager);

        /**
         * Method you can use to add a monitor to this host/scheme.  Monitors are tracked by monitor ID.  If a monitor
         * with the same monitor ID is already registered with this host/scheme, the existing monitor will be
         * deleted and replaced.
         *
         * The monitor's host/scheme instance will also be set to point to this host/scheme.
         *
         * Note that the host/scheme will take ownership of this monitor through Qt's inheritance mechanism.
         *
         * \param[in] monitor The monitor to be added.
         */
        void addMonitor(Monitor* monitor);

        /**
         * Method you can use to remove a monitor from this host/scheme.  The monitor will be destroyed.
         *
         * Note that you should always use this method rather than deleting the monitor directly.
         *
         * \param[in] monitorId The ID of the monitor to be removed.
         *
         * \return Returns true on success.  Returns false if the monitor is not tied to this host scheme instance.
         */
        bool removeMonitor(Monitor::MonitorId monitorId);

        /**
         * Method you can use to obtain a pointer to a monitor by monitor ID.
         *
         * \param[in] monitorId The ID of the monitor of interest.
         *
         * \return Returns the desired monitor.
         */
        Monitor* getMonitor(Monitor::MonitorId monitorId) const;

        /**
         * Method you can use to obtain all monitors under this customer.
         *
         * \return Returns a list of monitor instances.
         */
        QList<Monitor*> monitors() const;

        /**
         * Method you can use to obtain the host/scheme ID.
         *
         * \return Returns the region ID for this region.
         */
        HostSchemeId hostSchemeId() const;

        /**
         * Method you can use to obtain the customer instance owning this host/scheme.
         *
         * \return Returns the customer instance.  A null pointer may be returned if the Customer instance no longer
         *         exists.
         */
        Customer* customer() const;

        /**
         * Method you can use to change customer that owns this host/scheme.
         *
         * \param[in] newCustomer The new customer for this host/scheme.
         */
        void setCustomer(Customer* newCustomer);

        /**
         * Method you can use to obtain the URL for this host/scheme.
         *
         * \return Returns the URL.
         */
        const QUrl& url() const;

        /**
         * Method you can use to change the URL for this host/scheme.
         *
         * \param[in] newUrl The new customer ID for this host/scheme.
         */
        void setUrl(const QUrl& newUrl);

        /**
         * Method you can use to obtain the SSL certificate expiration timestamp.
         *
         * \return Returns the SSL expiration timestamp.
         */
        unsigned long long sslExpirationTimestamp() const;

        /**
         * Method you can use to set the SSL certificate expiration timestamp.
         *
         * \param[in] newSslExpirationTimestamp The new SSL expiration timestamp.
         */
        void setSslExpirationTimestamp(unsigned long long newSslExpirationTimestamp);

        /**
         * Method that determines the number of monitors under this host/scheme.
         *
         * \return Returns the number of monitors under this host/scheme.
         */
        inline unsigned numberMonitors() const {
            return static_cast<unsigned>(children().size());
        }

        /**
         * Method you can use to trigger this monitor from a different thread.
         */
        void startCheckFromDifferentThread();

    signals:
        /**
         * Signal that is emitted when a request has been made to immediately check this host/scheme from anther
         * thread.
         */
        void startCheckRequested();

    public slots:
        /**
         * Slot you can trigger to start servicing the next monitor under this host/scheme. This slot must be called
         * from the same thread as the timer, if invoked directly.
         */
        void serviceNextMonitor();

        /**
         * Slot you can trigger to report that a monitor is non-responsive.  Timing for non-responsive monitors will
         * be adjusted to detect their response more quickly.
         *
         * \param monitor Pointer to the non-responsive monitor.
         */
        void monitorNonResponsive(Monitor* monitor);

        /**
         * Slot you can trigger to report that a monitor is now responsive.  Timing for non-responsive monitors will
         * be adjusted to detect their response more quickly.
         *
         * This slot must either be triggered by a signal or called from the thread that owns this object.
         *
         * \param monitor Pointer to the non-responsive monitor.
         */
        void monitorNowResponsive(Monitor* monitor);

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
         * Method that is called by the customer to propagate information about monitors under this host scheme.
         *
         * \param[in] customer The customer that we've just been added to.
         *
         * \param[in] adding   If true, we're being added.  If false, we're being removed.
         */
        void reportExistingMonitors(Customer* customer, bool adding);

        /**
         * The current network access manager.
         */
        QNetworkAccessManager* currentNetworkAccessManager;

        /**
         * The host/scheme ID of this host scheme.
         */
        HostSchemeId currentHostSchemeId;

        /**
         * The URL used to access the server.  The URL will always exclude paths, query strings, and fragments.
         */
        QUrl currentUrl;

        /**
         * The last reported SSL expiration timestamp.
         */
        unsigned long long currentSslExpirationTimestamp;

        /**
         * Mutex used to protect our monitor hash table.
         */
        QMutex monitorMutex;

        /**
         * Set of non-responsive monitors.
         */
        QSet<Monitor*> nonResponsiveMonitors;

        /**
         * Iterator into the set of non-responsive monitors.
         */
        QSet<Monitor*>::iterator nonResponsiveMonitorsIterator;

        /**
         * Hash table of monitors under this host/scheme.
         */
        MonitorsByMonitorId monitorsByMonitorId;

        /**
         * Iterator used to cycle through the monitors under this host/scheme.
         */
        MonitorsByMonitorId::iterator monitorIterator;
};

#endif
