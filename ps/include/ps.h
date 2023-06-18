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
* This header defines the polling server main class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef POLLING_SERVER_H
#define POLLING_SERVER_H

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QFileSystemWatcher>
#include <QHostAddress>
#include <QSharedPointer> // DEBUG

#include "monitor.h"

namespace RestApiOutV1 {
    class Server;
    class InesonicRestHandler;
}

namespace RestApiInV1 {
    class Server;
    class TimeDeltaHandler;
}

class QNetworkAccessManager;
class ServiceThreadTracker;
class DataAggregator;
class InboundRestApi;

/**
 * The polling server application class.
 */
class PollingServer:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor
         *
         * \param[in] configurationFilename The configuration file used to manage database and application settings.
         *                                  Note that this file will be monitored for updates so it can be changed
         *                                  while the application is running.
         *
         * \param[in] parent                Pointer to the parent object.
         */
        PollingServer(const QString& configurationFilename, QObject* parent = nullptr);

        ~PollingServer() override;

    private slots:
        /**
         * Method that is triggered whenever the supplied configuration file is changed.
         *
         * \param[in] filePath The path to the changed file.
         */
        void configurationFileChanged(const QString& filePath);

    private:
        /**
         * Method that configures our polling server.
         *
         * \param[in] inboundApiKey    The inbound API key use to authenticate requests from the database server.
         *
         * \param[in] outboundApiKey   The API to use to talk to the database controller REST API.
         *
         * \param[in] databaseServer   The URL of the database controller REST API.
         *
         * \param[in] inboundPort      The inbound port number.
         *
         * \param[in] serverIdentifier A list of network addresses on this machine.
         *
         * \param[in] defaultHeaders   A map of default header key/value pairs.
         *
         * \param[in] pingerString     String used to connect to the pinger.
         */
        void configureServer(
            const QByteArray&       inboundApiKey,
            const QByteArray&       outboundApiKey,
            const QString&          databaseServer,
            unsigned short          inboundPort,
            const QString&          serverIdentifier,
            const Monitor::Headers& defaultHeaders,
            const QString&          pingerString
        );

        /**
         * The REST API key length, in bytes.
         */
        static constexpr unsigned keyLength = 56;

        /**
         * The filesystem watcher used to monitor the configuration file.
         */
        QFileSystemWatcher* fileSystemWatcher;

        /**
         * The path to the current configuration file.
         */
        QString currentConfigurationFilename;

        /**
         * Our global network access manager instance.
         */
        QNetworkAccessManager* currentNetworkAccessManager;

        /**
         * The inbound REST API server -- This server receives instruction from the database controller.
         */
        RestApiInV1::Server* inboundRestApiServer;

        /**
         * The time delta handler for inbound requests.
         */
        RestApiInV1::TimeDeltaHandler* timeDeltaHandler;

        /**
         * The REST API outbound server -- This server connects to the database controller.
         */
        RestApiOutV1::Server* outboundRestApiServer;

        /**
         * Our global data aggregator.
         */
        DataAggregator* dataAggregator;

        /**
         * Our test service thread.
         */
        ServiceThreadTracker* serviceThreadTracker;

        /**
         * The inbound REST API manager.
         */
        InboundRestApi* inboundRestApi;
};

#endif
