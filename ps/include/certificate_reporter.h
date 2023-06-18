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
* This header defines the \ref CertificateReporter class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef CERTIFICATE_REPORTER_H
#define CERTIFICATE_REPORTER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QJsonObject>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "host_scheme.h"
#include "monitor.h"

/**
 * Class that performs asynchronous reporting of an update certificate expiration date/time.  This class is expected to
 * be dynamically allocated and will delete itself automatically upon successful reporting.
 *
 * To use, instantiate an instance and then call the \ref startReporting method (or trigger it).
 */
class CertificateReporter:public RestApiOutV1::InesonicRestHandler {
    Q_OBJECT

    public:
        /**
         * The endpoint we should report events to.
         */
        static const QString certificateReportPath;

        /**
         * Type used to represent a monitor ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Constructor
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        CertificateReporter(RestApiOutV1::Server* server, QObject* parent = nullptr);

        /**
         * Constructor
         *
         * \param[in] secret The secret to be used by this REST API.
         *
         * \param[in] server The server instance this REST API will talk to.
         *
         * \param[in] parent Pointer to the parent object.
         */
        CertificateReporter(const QByteArray& secret, RestApiOutV1::Server* server, QObject* parent = nullptr);

    public slots:
        /**
         * Slot you can trigger to initiate reporting.
         *
         * \param[in] monitorId           The monitor ID of the monitor that detected the event.
         *
         * \param[in] hostSchemeId        The host/scheme ID of the host scheme tied to this certificate.
         *
         * \param[in] expirationTimestamp The expiration timestamp for this certificate.
         */
        void startReporting(
            MonitorId          monitorId,
            HostSchemeId       hostSchemeId,
            unsigned long long expirationTimestamp
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
         * The JSON message to be sent.
         */
        QJsonObject jsonMessage;

        /**
         * Timer used to retry sending requests.
         */
        QTimer retryTimer;
};

#endif
