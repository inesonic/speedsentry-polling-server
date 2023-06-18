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
* This header implements the polling seerver main class.
***********************************************************************************************************************/

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QFileSystemWatcher>
#include <QFile>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkInterface>

#include <cstring>

#include <crypto_helpers.h>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_time_delta_handler.h>

#include <rest_api_out_v1_server.h>

#include "log.h"
#include "resources.h"
#include "monitor.h"
#include "host_scheme.h"
#include "customer.h"
#include "data_aggregator.h"
#include "service_thread_tracker.h"
#include "inbound_rest_api.h"
#include "ps.h"

PollingServer::PollingServer(
        const QString& configurationFilename,
        QObject*       parent
    ):QObject(
        parent
    ) {
    currentConfigurationFilename = configurationFilename;
    fileSystemWatcher            = new QFileSystemWatcher(QStringList() << configurationFilename, this);

    currentNetworkAccessManager = new QNetworkAccessManager(this);

    inboundRestApiServer  = new RestApiInV1::Server(1, this);
    inboundRestApiServer->setLoggingFunction(&logWrite);

    timeDeltaHandler = new RestApiInV1::TimeDeltaHandler;
    inboundRestApiServer->registerHandler(
        timeDeltaHandler,
        RestApiInV1::Handler::Method::POST,
        RestApiInV1::TimeDeltaHandler::defaultEndpoint
    );

    outboundRestApiServer = new RestApiOutV1::Server(
        currentNetworkAccessManager,
        QUrl(),
        RestApiOutV1::Server::defaultTimeDeltaSlug,
        this
    );

    dataAggregator = new DataAggregator(outboundRestApiServer, this);
    serviceThreadTracker = new ServiceThreadTracker(dataAggregator, 0, this);
    dataAggregator->setServiceThreadTracker(serviceThreadTracker);

    inboundRestApi = new InboundRestApi(inboundRestApiServer, serviceThreadTracker, QByteArray(), this);

    connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &PollingServer::configurationFileChanged);
    configurationFileChanged(configurationFilename);
}


PollingServer::~PollingServer() {
}


void PollingServer::configurationFileChanged(const QString& /* filePath */) {
    QFile configurationFile(currentConfigurationFilename);
    bool success = configurationFile.open(QFile::OpenModeFlag::ReadOnly);
    if (success) {
        QByteArray configurationData = configurationFile.readAll();
        configurationFile.close();

        QJsonParseError jsonParseResult;
        QJsonDocument   jsonDocument = QJsonDocument::fromJson(configurationData, &jsonParseResult);

        if (jsonParseResult.error == QJsonParseError::ParseError::NoError) {
            QJsonObject jsonObject = jsonDocument.object();

            QString    encodedInboundApiKey  = jsonObject.value(QString("inbound_api_key")).toString();
            QString    encodedOutboundApiKey = jsonObject.value(QString("outbound_api_key")).toString();
            QString    databaseServer        = jsonObject.value(QString("database_server")).toString();
            long       inboundPort           = jsonObject.value(QString("inbound_port")).toInt(-1);
            QString    serverIdentifier      = jsonObject.value(QString("server_identifier")).toString();

            QJsonValue headerStrings         =   jsonObject.contains(QString("headers"))
                                               ? jsonObject.value(QString("headers"))
                                               : QJsonValue(QJsonObject());

            QString    pingerString          = jsonObject.value("pinger").toString("Pinger");

            QByteArray::FromBase64Result inboundKey = QByteArray::fromBase64Encoding(
                encodedInboundApiKey.toUtf8(),
                QByteArray::Base64Option::Base64Encoding
            );

            if (inboundKey) {
                QByteArray inboundApiKey = *inboundKey;
                if (static_cast<unsigned>(inboundApiKey.size()) == keyLength) {
                    QByteArray::FromBase64Result outboundKey = QByteArray::fromBase64Encoding(
                        encodedOutboundApiKey.toUtf8(),
                        QByteArray::Base64Option::Base64Encoding
                    );
                    if (outboundKey) {
                        QByteArray outboundApiKey = *outboundKey;
                        if (static_cast<unsigned>(outboundApiKey.size()) == keyLength) {
                            if (inboundPort > 0 && inboundPort <= 0xFFFF) {
                                if (!serverIdentifier.isEmpty()) {
                                    Monitor::Headers headers;
                                    bool             success = headerStrings.isObject();
                                    if (success) {
                                        QJsonObject headerStringsObject = headerStrings.toObject();

                                        QJsonObject::const_iterator it  = headerStringsObject.constBegin();
                                        QJsonObject::const_iterator end = headerStringsObject.constEnd();
                                        while (success && it != end) {
                                            if (it.value().isString()) {
                                                const QString& headerName  = it.key();
                                                QString        headerValue = it.value().toString();
                                                headers.insert(headerName, headerValue);

                                                ++it;
                                            } else {
                                                success = false;
                                            }
                                        }
                                    }

                                    if (success) {
                                        configureServer(
                                            inboundApiKey,
                                            outboundApiKey,
                                            databaseServer,
                                            static_cast<unsigned short>(inboundPort),
                                            serverIdentifier,
                                            headers,
                                            pingerString
                                        );
                                    } else {
                                        logWrite(QString("Invalid header data."), true);
                                        success = false;
                                    }
                                } else {
                                    logWrite(QString("Invalid server identifier."), true);
                                    success = false;
                                }
                            } else {
                                logWrite(QString("Invalid inbound port."), true);
                                success = false;
                            }
                        } else {
                            logWrite(QString("Invalid outbound API key length."), true);
                            success = false;
                        }

                        Crypto::scrub(outboundApiKey);
                    }
                } else {
                    logWrite(QString("Invalid inbound API key length."), true);
                    success = false;
                }

                Crypto::scrub(inboundApiKey);
            } else {
                logWrite(QString("Invalid API key length."), true);
                success = false;
            }
        } else {
            logWrite(QString("Invalid JSON formatted configuration file."), true);
            success = false;
        }
    } else {
        logWrite(QString("Could not open configuration file %1").arg(currentConfigurationFilename), true);
        success = false;
    }

    if (!success) {
        QCoreApplication::instance()->exit(1);
    }
}


void PollingServer::configureServer(
        const QByteArray&       inboundApiKey,
        const QByteArray&       outboundApiKey,
        const QString&          databaseServer,
        unsigned short          inboundPort,
        const QString&          serverIdentifier,
        const Monitor::Headers& defaultHeaders,
        const QString&          pingerString
    ) {
    inboundRestApiServer->reconfigure(RestApiInV1::Server::defaultHostAddress, inboundPort);
    inboundRestApi->setSecret(inboundApiKey);

    outboundRestApiServer->setSchemeAndHost(QUrl(databaseServer));
    outboundRestApiServer->setDefaultSecret(outboundApiKey);

    dataAggregator->setServerIdentifier(serverIdentifier);
    serviceThreadTracker->connectToPinger(pingerString);

    Monitor::setDefaultHeaders(defaultHeaders);
}
