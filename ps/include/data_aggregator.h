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
* This header defines the \ref DataAggregator class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef DATA_AGGREGATOR_H
#define DATA_AGGREGATOR_H

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QMutex>
#include <QTimer>

#include <cstdint>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_binary_rest_handler.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "metatypes.h"
#include "host_scheme.h"
#include "monitor.h"

class ServiceThreadTracker;
class EventReporter;

/**
 * Class that collects data from each monitor, including latency information and error reports.
 */
class DataAggregator:public RestApiOutV1::InesonicBinaryRestHandler {
    Q_OBJECT

    public:
        /**
         * Type used to represent a monitor ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Type used to represent an event type.
         */
        typedef HostScheme::EventType EventType;

        /**
         * Type used to report an event.
         */
        typedef Monitor::MonitorStatus MonitorStatus;

        /**
         * Type used to represent a timestamp.
         */
        typedef std::uint32_t ZoranTimeStamp;

        /**
         * Type used to represent a stored latency value.  Value is in microseconds.
         */
        typedef std::uint32_t LatencyMicroseconds;

        /**
         * Value indicating the correction between the start of the Zoran epoch and the Unix epoch.
         *
         * The value translates to seconds between 00:00:00 January 1, 2021 and 00:00:00 January 1, 1970.
         */
        static constexpr std::uint64_t startOfZoranEpoch = 1609484400;

        /**
         * The endpoint used to report latency data.
         */
        static const QString latencyRecordPath;

        /**
         * Class used to track a single latency entry.
         */
        class LatencyEntry {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] monitorId           The ID of the monitor that took this measurement.
                 *
                 * \param[in] unixTimestamp       The Unix timestamp.
                 *
                 * \param[in] latencyMicroseconds The latency measurement, in microseconds.
                 */
                constexpr LatencyEntry(
                        MonitorId           monitorId,
                        unsigned long long  unixTimestamp,
                        LatencyMicroseconds latencyMicroseconds
                    ):currentMonitorId(
                        monitorId
                    ),currentTimestamp(
                        unixTimestamp - startOfZoranEpoch
                    ),currentLatencyMicroseconds(
                        latencyMicroseconds
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                constexpr LatencyEntry(
                        const LatencyEntry& other
                    ):currentMonitorId(
                        other.currentMonitorId
                    ),currentTimestamp(
                        other.currentTimestamp
                    ),currentLatencyMicroseconds(
                        other.currentLatencyMicroseconds
                    ) {}

                ~LatencyEntry() = default;

                /**
                 * Method you can use to obtain the monitor ID.
                 *
                 * \return Returns the monitor ID.
                 */
                inline MonitorId monitorId() const {
                    return currentMonitorId;
                }

                /**
                 * Method you can use to obtain the current Zoran timestamp.
                 *
                 * \return Returns the current timestamp in Zoran time.
                 */
                inline ZoranTimeStamp zoranTimestamp() const {
                    return currentTimestamp;
                }

                /**
                 * Method you can use to obtain the current Unix timestamp for this entry.
                 *
                 * \return Returns the current Unix timestamp for this entry.
                 */
                inline std::uint64_t unixTimestamp() const {
                    return currentTimestamp + startOfZoranEpoch;
                }

                /**
                 * Method you can use to obtain the current latency value, in microseconds.
                 *
                 * \return Returns the current latency value, in microseconds.
                 */
                inline LatencyMicroseconds latencyMicroseconds() const {
                    return currentLatencyMicroseconds;
                }

                /**
                 * Method you can use to obtain the current latency value, in seconds.
                 *
                 * \return Returns the current latency value, in seconds.
                 */
                inline double latencySeconds() const {
                    return currentLatencyMicroseconds / 1000000.0;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline LatencyEntry& operator=(const LatencyEntry& other) {
                    currentMonitorId           = other.currentMonitorId;
                    currentTimestamp           = other.currentTimestamp;
                    currentLatencyMicroseconds = other.currentLatencyMicroseconds;

                    return *this;
                }

                /**
                 * Method you can use to convert Zoran time to Unix time.
                 *
                 * \param[in] zoranTimestamp The Zoran timestamp to be converted.
                 *
                 * \return Returns the resulting Unix timestamp.
                 */
                static inline std::uint64_t toUnixTimestamp(ZoranTimeStamp zoranTimestamp) {
                    return zoranTimestamp + startOfZoranEpoch;
                }

                /**
                 * Method you can use to convert Unix time to Zoran time.
                 *
                 * \param[in] unixTimestamp The Unix timestamp to be converted.
                 *
                 * \return Returns the resulting Zoran timestamp.
                 */
                static inline ZoranTimeStamp toZoranTimestamp(std::uint64_t unixTimestamp) {
                    return static_cast<ZoranTimeStamp>(unixTimestamp - startOfZoranEpoch);
                }

            private:
                /**
                 * The monitor ID.
                 */
                MonitorId currentMonitorId;

                /**
                 * The timestamp value being tracked.
                 */
                ZoranTimeStamp currentTimestamp;

                /**
                 * The current latency value being tracked, in microseconds.
                 */
                LatencyMicroseconds currentLatencyMicroseconds;
        };

        /**
         * Type used to contain a collection of latency entries.
         */
        typedef QVector<LatencyEntry> LatencyEntryList;

        /**
         * Constructor.
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        DataAggregator(RestApiOutV1::Server* server, QObject* parent = nullptr);

        /**
         * Constructor
         *
         * \param[in] secret The secret to be used by this REST API.
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        DataAggregator(const QByteArray& secret, RestApiOutV1::Server* server, QObject* parent = nullptr);

        /**
         * Constructor.
         *
         * \param[in] serviceThreadTracker The service thread tracker used to manage work.
         *
         * \param[in] server               The server instance this REST API will talk to.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        DataAggregator(
            ServiceThreadTracker* serviceThreadTracker,
            RestApiOutV1::Server* server,
            QObject*              parent = nullptr
        );

        /**
         * Constructor
         *
         * \param[in] serviceThreadTracker The service thread tracker used to manage work.
         *
         * \param[in] secret               The secret to be used by this REST API.
         *
         * \param[in] server               The server instance this REST API will talk to.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        DataAggregator(
            ServiceThreadTracker* serviceThreadTracker,
            const QByteArray&     secret,
            RestApiOutV1::Server* server,
            QObject*              parent = nullptr
        );

        ~DataAggregator() override;

        /**
         * Method you can use to obtain the service thread tracker this class is using.
         *
         * \return Returns a pointer to the service thread tracker being used by this class.
         */
        ServiceThreadTracker* serviceThreadTracker() const;

        /**
         * Method you can use to set the service thread tracker to be used by this class.
         *
         * \param[in] newServiceThreadTracker The service thread tracker used to manage work.
         */
        void setServiceThreadTracker(ServiceThreadTracker* newServiceThreadTracker);

        /**
         * Method you can use to obtain the system's server identifier.
         *
         * \return Returns the current server identifier.
         */
        QString serverIdentifier() const;

        /**
         * Method you can use to set the system's server identifier.
         *
         * \param[in] newServerIdentifier The new server identifier.
         */
        void setServerIdentifier(const QString& newServerIdentifier);

        /**
         * Method that can be called by a thread to report new latency data.  This method is fully thread safe.
         *
         * \param[in] monitorId The monitor ID of the monitor making the report.
         *
         * \param[in] timestamp The Unix timestamp indicating when the request was triggered.
         *
         * \param[in] latency   The reported latency, in microseconds.
         */
        void recordLatency(Monitor::MonitorId monitorId, unsigned long long timestamp, LatencyMicroseconds latency);

        /**
         * Method that can be called to report an event.  This method is fully thread safe.
         *
         * \param[in] monitorId     The monitor ID of the monitor that detected the event.
         *
         * \param[in] timestamp     A timestamp indicating the event.
         *
         * \param[in] eventType     The type of event detected.
         *
         * \param[in] monitorStatus The monitor status when this event was triggered.
         *
         * \param[in] hash          The cryptographic hash of the page or discovered keywords.  This data is used to
         *                          block
         *                          repeated reports of the same data.
         *
         * \param[in] message       A brief description of the event.
         */
        void reportEvent(
            MonitorId          monitorId,
            unsigned long long timestamp,
            EventType          eventType,
            MonitorStatus      monitorStatus,
            const QByteArray&  hash,
            const QString&     message = QString()
        );

        /**
         * Method that can be called to report a new SSL certificate expiration date/time.
         *
         * \param[in] monitorId              The monitor ID of the monitor that detected the change.
         *
         * \param[in] hostSchemeId           The host/scheme that the certificate is tied to.
         *
         * \param[in] newExpirationTimestamp The new expiration timestamp in seconds since start of the Unix epoch.
         */
        void reportSslCertificateExpirationChange(
            MonitorId          monitorId,
            HostSchemeId       hostSchemeId,
            unsigned long long newExpirationTimestamp
        );

    signals:
        /**
         * Signal you can use to start the report timer across threads.
         *
         * \param[out] delay The desired delay, in milliseconds.
         */
        void triggerReporting(unsigned long delay = 0);

        /**
         * Signal you can use to start a retrial of a pending transmission request across threads.
         *
         * \param[out] delay The desired delay, in milliseconds.
         */
        void triggerRetry(unsigned long delay = 0);

        /**
         * Signal that is emitted to report an event.
         *
         * \param[out] monitorId     The monitor ID of the monitor that detected the event.
         *
         * \param[out] timestamp     A timestamp indicating the event.
         *
         * \param[out] eventType     The type of event detected.  Value is cast to an integer to address Qt metatype
         *                           issues.
         *
         * \param[out] monitorStatus The monitor status when the event was triggered.
         *
         * \param[out] hash          The cryptographic hash of the page or discovered keywords.  This data is used to
         *                           block repeated reports of the same data.
         *
         * \param[out] message       A brief description of the event.
         */
        void reportEventRequested(
            unsigned long      monitorId,
            unsigned long long timestamp,
            unsigned           eventType,
            unsigned           monitorStatus,
            const QByteArray&  hash,
            const QString&     message = QString()
        );

        /**
         * Signal that is emitted to report a new SSL certificate expiration date/time.
         *
         * \param[out] monitorId              The monitor ID of the monitor that detected the change.
         *
         * \param[out] hostSchemeId           The host/scheme that the certificate is tied to.
         *
         * \param[out] newExpirationTimestamp The new expiration timestamp in seconds since start of the Unix epoch.
         */
        void reportSslCertificateExpirationChangeRequested(
            unsigned long      monitorId,
            unsigned long      hostSchemeId,
            unsigned long long newExpirationTimestamp
        );

    public slots:
        /**
         * Slot you can use to trigger the data aggregator to send a report immediately.  You can call this method from
         * any thread.
         */
        void sendReport();

    private slots:
        /**
         * Slot that is triggered to report an event.
         *
         * \param[in] monitorId      The monitor ID of the monitor that detected the event.
         *
         * \param[in] timestamp      A timestamp indicating the event.
         *
         * \param[in] eventType      The type of event detected.
         *
         * \param[out] monitorStatus The monitor status when the event was triggered.
         *
         * \param[in] hash           The cryptographic hash of the page or discovered keywords.  This data is used to
         *                           block repeated reports of the same data.
         *
         * \param[in] message        A brief description of the event.
         */
        void processReportEvent(
            unsigned long      monitorId,
            unsigned long long timestamp,
            unsigned           eventType,
            unsigned           monitorStatus,
            const QByteArray&  hash,
            const QString&     message = QString()
        );

        /**
         * Slot that is triggered to report a new SSL certificate expiration date/time.
         *
         * \param[in] monitorId              The monitor ID of the monitor that detected the change.
         *
         * \param[in] hostSchemeId           The host/scheme that the certificate is tied to.
         *
         * \param[in] newExpirationTimestamp The new expiration timestamp in seconds since start of the Unix epoch.
         */
        void processReportSslCertificateExpirationChange(
            unsigned long      monitorId,
            unsigned long      hostSchemeId,
            unsigned long long newExpirationTimestamp
        );

    protected:
        /**
         * Method you can overload to process a received response.  The default implementation will trigger the
         * \ref responseReceived signal.
         *
         * \param[in] binaryData  The received binary response.
         *
         * \param[in] contentType The response content type, if reported.
         */
        void processResponse(const QByteArray& binaryData, const QString& contentType) override;

        /**
         * Method you can overload to process a failed transmisison attempt.  The default implementation will
         * trigger the \ref requestFailed signal.
         *
         * \param[in] errorString a string providing an error message.
         */
        void processRequestFailed(const QString& errorString) override;

    private slots:
        /**
         * Slot that is triggered to start reporting latency data.
         */
        void startReportingLatencyData();

        /**
         * Slot that is triggered to start a transmission re-attempt.
         */
        void startRetry();

    private:
        /**
         * The maximum time to wait before starting a report of new latency entries.
         */
        static constexpr unsigned long maximumReportDelayMilliseconds = 60 * 1000; // 5 * 60 * 1000;

        /**
         * The maximum number of stale entries allowed before we start reporting.
         */
        static constexpr unsigned long maximumNumberPendingEntries = 1000;

        /**
         * The delay before retrying to send data, in seconds.
         */
        static constexpr unsigned retryDelaySeconds = 60;

        /**
         * The server maximum identifier length, in bytes.
         */
        static constexpr unsigned maximumIdentifierLength = 48;

        /**
         * Structure that defines our latency record header.  Note that this structure is also defined in the
         * db_controller project with a structure that must match this one.
         */
        struct Header {
            /**
             * A header version code value.  Currently ignored and set to 0.
             */
            std::uint16_t version;

            /**
             * The identifier for this server, encoded in UTF-8 format.
             */
            std::uint8_t identifier[maximumIdentifierLength];

            /**
             * The monitor service rate for this service.  The value holds the number of monitors serviced per
             * second.  Value is in unsigned 24.8 notation.
             */
            std::uint32_t monitorsPerSecond;

            /**
             * The CPU loading reported as a value between 0 and 65535 where 0 is 0% and 65535 is 1600%.
             */
            std::uint16_t cpuLoading;

            /**
             * The memory loading reported as a value between 0 and 65535 where 0 is 0% and 65535 is 100%.
             */
            std::uint16_t memoryLoading;

            /**
             * The current server status.
             */
            std::uint8_t serverStatusCode;

            /**
             * Reserved for future use.  Fill with zeros.
             */
            std::uint8_t spare[64 - (2 + maximumIdentifierLength + 4 + 2 + 2 + 1)];
        } __attribute__((packed));

        /**
         * Structure that defines our latency entry.  Note that this structure is also defined in the db_controller
         * project with a structure that must match this one.
         */
        struct Entry {
            /**
             * The monitor ID.
             */
            std::uint32_t monitorId;

            /**
             * The Zoran timestamp.
             */
            std::uint32_t timestamp;

            /**
             * The latency in microseconds
             */
            std::uint32_t latencyMicroseconds;
        } __attribute__((packed));

        /**
         * Method used to perform common initialization of this class.
         */
        void configure();

        /**
         * Method that builds and sends a latency report.
         *
         * \param[in] latencyEntryList The list of latency entries to be sent.
         */
        void sendReport(const LatencyEntryList& latencyEntryList);

        /**
         * Class used to report events.
         */
        EventReporter* eventReporter;

        /**
         * Pointer to our service thread tracker.
         */
        ServiceThreadTracker* currentServiceThreadTracker;

        /**
         * A template of our header that we use to more quickly build messages.  The template has been pre-zeroed and
         * has the server identifier already populated.
         */
        Header headerTemplate;

        /**
         * Mutex used to guard the latency event list.
         */
        QMutex listMutex;

        /**
         * The current incoming latency entry list.
         */
        LatencyEntryList* latencyEntryList;

        /**
         * The in-flight latency entry list.
         */
        LatencyEntryList* inFlightLatencyEntryList;

        /**
         * Timer used to report latency data at periodic intervals.
         */
        QTimer* reportTimer;

        /**
         * Timer used to retry transmission of latency data.
         */
        QTimer* retryTimer;
};

#endif
