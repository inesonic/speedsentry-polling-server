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
* This header implements the \ref CertificateReporter class.
***********************************************************************************************************************/

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include "log.h"
#include "host_scheme.h"
#include "monitor.h"
#include "certificate_reporter.h"

const QString CertificateReporter::certificateReportPath("/host_scheme/certificate");

CertificateReporter::CertificateReporter(
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicRestHandler(
        server,
        parent
    ) {
    retryTimer.setSingleShot(true);
    connect(&retryTimer, &QTimer::timeout, this, &CertificateReporter::resend);
}


CertificateReporter::CertificateReporter(
        const QByteArray&     secret,
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicRestHandler(
        secret,
        server,
        parent
    ) {}


void CertificateReporter::startReporting(
        MonitorId          monitorId,
        HostSchemeId       hostSchemeId,
        unsigned long long expirationTimestamp
    ) {
    jsonMessage.insert("monitor_id", static_cast<double>(monitorId));
    jsonMessage.insert("host_scheme_id", static_cast<double>(hostSchemeId));
    jsonMessage.insert("expiration_timestamp", static_cast<double>(expirationTimestamp));

    resend();
}


void CertificateReporter::processJsonResponse(const QJsonDocument& jsonData) {
    if (jsonData.isObject()) {
        QJsonObject responseObject = jsonData.object();
        if (responseObject.size() == 1) {
            QString statusString = responseObject.value("status").toString();
            if (statusString == QString("OK")) {
                QByteArray message = QJsonDocument(jsonMessage).toJson(QJsonDocument::JsonFormat::Compact);
                logWrite(
                    QString("Sent certificate data to %1: %2").arg(certificateReportPath, QString::fromUtf8(message)),
                    false
                );

                this->deleteLater();
            } else {
                processRequestFailed(QString("Server reported \"%1\"").arg(statusString));
            }
        } else {
            processRequestFailed(QString("Unexpected response"));
        }
    } else {
        processRequestFailed(QString("Expected JSON object."));
    }
}


void CertificateReporter::processRequestFailed(const QString& errorString) {
    logWrite(
        QString("Failed to send certificate data: %1 -- retrying in %2 seconds")
        .arg(errorString)
        .arg(retryDelayInSeconds),
        true
    );

    retryTimer.start(retryDelayInSeconds * 1000);
}


void CertificateReporter::resend() {
    post(certificateReportPath, jsonMessage);
}
