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
* This header defines the \ref EventReporter class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef EVENT_REPORTER_H
#define EVENT_REPORTER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "host_scheme.h"
#include "monitor.h"

/**
 * Class that performs asynchronous reporting of an event.  This class is expected to be dynamically allocated and
 * will delete itself automatically upon successful event reporting.
 *
 * To use, instantiate an instance and then call the \ref startReporting method (or trigger it).
 */
class EventReporter:public RestApiOutV1::InesonicRestHandler {
    Q_OBJECT

    public:
        /**
         * The endpoint we should report events to.
         */
        static const QString eventReportPath;

        /**
         * Type used to represent an event type.
         */
        typedef HostScheme::EventType EventType;

        /**
         * Type used to represent the status of a monitor.
         */
        typedef Monitor::MonitorStatus MonitorStatus;

        /**
         * Type used to represent a monitor ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Constructor
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        EventReporter(RestApiOutV1::Server* server, QObject* parent = nullptr);

        /**
         * Constructor
         *
         * \param[in] secret The secret to be used by this REST API.
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        EventReporter(const QByteArray& secret, RestApiOutV1::Server* server, QObject* parent = nullptr);

    public slots:
        /**
         * Slot you can trigger to initiate reporting.
         *
         * \param[in] monitorId     The monitor ID of the monitor that detected the event.
         *
         * \param[in] timestamp     A timestamp indicating the event.
         *
         * \param[in] eventType     The type of event detected.
         *
         * \param[in] monitorStatus The monitor status of the monitor when the event occurred.
         *
         * \param[in] hash          The cryptographic hash of the page or discovered keywords.  This data is used to
         *                          block repeated reports of the same data.
         *
         * \param[in] message       A brief description of the event.
         */
        void sendEvent(
            MonitorId          monitorId,
            unsigned long long timestamp,
            EventType          eventType,
            MonitorStatus      monitorStatus,
            const QByteArray&  hash,
            const QString&     message = QString()
        );

    protected:
        /**
         * Method you can overload to process a received response.  The default implementation will trigger the
         * \ref jsonResponse signal.
         *
         * \param[in] jsonData The received JSON response.
         */
        void processJsonResponse(const QJsonDocument& jsonData) override;

        /**
         * Method you can overload to process a failed transmisison attempt.  The default implementation will
         * trigger the \ref requestFailed signal.
         *
         * \param[in] errorString a string providing an error message.
         */
        void processRequestFailed(const QString& errorString) override;

    private slots:
        /**
         * Slot that is triggered to resend this request.
         */
        void resend();

    private:
        /**
         * The delay before retrying this event.
         */
        static const unsigned retryDelayInSeconds = 60;

        /**
         * Class that holds information about a pending message.
         */
        class Message {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] jsonMessage    The JSON message to be sent.
                 *
                 * \param[in] successMessage The message to show in the log on success.
                 *
                 * \param[in] failureMessage The message to show in the log on failure.
                 */
                inline Message(
                        const QJsonDocument& jsonMessage,
                        const QString&       successMessage,
                        const QString&       failureMessage
                    ):currentJsonMessage(
                        jsonMessage
                    ),currentSuccessMessage(
                        successMessage
                    ),currentFailureMessage(
                        failureMessage
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] jsonMessage    The JSON message to be sent.
                 *
                 * \param[in] successMessage The message to show in the log on success.
                 *
                 * \param[in] failureMessage The message to show in the log on failure.
                 */
                inline Message(
                        const Message& other
                    ):currentJsonMessage(
                        other.currentJsonMessage
                    ),currentSuccessMessage(
                        other.currentSuccessMessage
                    ),currentFailureMessage(
                        other.currentFailureMessage
                    ) {}

                /**
                 * Copy constructor (move semantics)
                 *
                 * \param[in] jsonMessage    The JSON message to be sent.
                 *
                 * \param[in] successMessage The message to show in the log on success.
                 *
                 * \param[in] failureMessage The message to show in the log on failure.
                 */
                inline Message(
                        Message&& other
                    ):currentJsonMessage(
                        other.currentJsonMessage
                    ),currentSuccessMessage(
                        other.currentSuccessMessage
                    ),currentFailureMessage(
                        other.currentFailureMessage
                    ) {}

                /**
                 * Method that provides the current JSON encoded message.
                 *
                 * \return Returns the current JSON encoded message.
                 */
                inline const QJsonDocument& message() const {
                    return currentJsonMessage;
                }

                /**
                 * Method that provides the current success message.
                 *
                 * \return Returns the current success message.
                 */
                inline const QString& successMessage() const {
                    return currentSuccessMessage;
                }

                /**
                 * Method that provides the current failure message.
                 *
                 * \return Returns the current failure message.
                 */
                inline const QString& failureMessage() const {
                    return currentFailureMessage;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Message& operator=(const Message& other) {
                    currentJsonMessage    = other.currentJsonMessage;
                    currentSuccessMessage = other.currentSuccessMessage;
                    currentFailureMessage = other.currentFailureMessage;

                    return *this;
                }

                /**
                 * Assignment operator (move semantics)
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Message& operator=(Message&& other) {
                    currentJsonMessage    = other.currentJsonMessage;
                    currentSuccessMessage = other.currentSuccessMessage;
                    currentFailureMessage = other.currentFailureMessage;

                    return *this;
                }

            private:
                /**
                 * The pending JSON message.
                 */
                QJsonDocument currentJsonMessage;

                /**
                 * The current success message.
                 */
                QString currentSuccessMessage;

                /**
                 * The current failure message.
                 */
                QString currentFailureMessage;
        };

        /**
         * Method that converts an event type to a string.
         *
         * \param[in] eventType The event type to be converted.
         *
         * \return Returns the event type as a lower case string.
         */
        static QString toString(EventType eventType);

        /**
         * Method that converts monitor status to a string.
         *
         * \param[in] monitorStatus The monitor status to be converted.
         *
         * \return Returns the monitor status as a lower case string.
         */
        static QString toString(MonitorStatus eventType);

        /**
         * Timer used to retry sending requests.
         */
        QTimer retryTimer;

        /**
         * List of pending messages.
         */
        QList<Message> pendingMessages;
};

#endif
