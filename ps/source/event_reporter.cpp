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
* This header implements the \ref EventReporter class.
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
#include "event_reporter.h"

const QString EventReporter::eventReportPath("/event/report");

EventReporter::EventReporter(
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicRestHandler(
        server,
        parent
    ) {
    retryTimer.setSingleShot(true);
    connect(&retryTimer, &QTimer::timeout, this, &EventReporter::resend);
}


EventReporter::EventReporter(
        const QByteArray&     secret,
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicRestHandler(
        secret,
        server,
        parent
    ) {}


void EventReporter::sendEvent(
        EventReporter::MonitorId     monitorId,
        unsigned long long           timestamp,
        EventReporter::EventType     eventType,
        EventReporter::MonitorStatus monitorStatus,
        const QByteArray&            hash,
        const QString&               message
    ) {
    bool startSending = pendingMessages.isEmpty();

    QJsonObject jsonMessage;
    QString     eventTypeString     = toString(eventType);
    QString     monitorStatusString = toString(monitorStatus);

    jsonMessage.insert("monitor_id", static_cast<double>(monitorId));
    jsonMessage.insert("timestamp", static_cast<double>(timestamp));
    jsonMessage.insert("event_type", eventTypeString);
    jsonMessage.insert("monitor_status", monitorStatusString);
    jsonMessage.insert("message", message);

    if (!hash.isEmpty()) {
        jsonMessage.insert("hash", QString::fromUtf8(hash.toBase64()));
    }

    QString successMessage = QString("Sent event %1 @ %2, (status %3) monitor ID %4, \"%5\"")
                             .arg(eventTypeString)
                             .arg(timestamp)
                             .arg(monitorStatusString)
                             .arg(monitorId)
                             .arg(message);

    QString failureMessage = QString("Failed to send event %1 @ %2 (status %3), monitor ID %4, \"%5\"")
                             .arg(eventTypeString)
                             .arg(timestamp)
                             .arg(monitorStatusString)
                             .arg(monitorId)
                             .arg(message);

    pendingMessages.append(Message(QJsonDocument(jsonMessage), successMessage, failureMessage));

    if (startSending) {
        post(eventReportPath, pendingMessages.first().message());
    }
}


void EventReporter::processJsonResponse(const QJsonDocument& jsonData) {
    Message m = pendingMessages.takeFirst();

    if (jsonData.isObject()) {
        QJsonObject responseObject = jsonData.object();
        if (responseObject.size() == 1) {
            QString statusString = responseObject.value("status").toString();
            if (statusString == QString("OK")) {
                logWrite(m.successMessage(), false);
            } else {
                logWrite(m.failureMessage() + QString(": Server reported \"%1\"").arg(statusString), false);
            }
        } else {
            logWrite(m.failureMessage() + QString(": Unexpected response"), false);
        }
    } else {
        logWrite(m.failureMessage() + QString(": Expected JSON object."), false);
    }

    resend();
}


void EventReporter::processRequestFailed(const QString& errorString) {
    const Message& m = pendingMessages.first();

    logWrite(
        m.failureMessage() + QString(": %1 - Retrying in %2 seconds.").arg(errorString).arg(retryDelayInSeconds),
        false
    );

    retryTimer.start(1000 * retryDelayInSeconds);
}


void EventReporter::resend() {
    if (!pendingMessages.isEmpty()) {
        post(eventReportPath, pendingMessages.first().message());
    }
}


QString EventReporter::toString(EventReporter::EventType eventType) {
    QString result;

    switch (eventType) {
        case EventType::INVALID:         { result = QString("invalid");          break; }
        case EventType::WORKING:         { result = QString("working");          break; }
        case EventType::NO_RESPONSE:     { result = QString("no_response");      break; }
        case EventType::CONTENT_CHANGED: { result = QString("content_changed");  break; }
        case EventType::KEYWORDS:        { result = QString("keywords");         break; }
        case EventType::SSL_CERTIFICATE: { result = QString("ssl_certificate");  break; }
        default:                         { Q_ASSERT(false);                      break; }
    }

    return result;
}


QString EventReporter::toString(EventReporter::MonitorStatus monitorStatus) {
    QString result;

    switch (monitorStatus) {
        case MonitorStatus::UNKNOWN: { result = QString("unknown");  break; }
        case MonitorStatus::WORKING: { result = QString("working");  break; }
        case MonitorStatus::FAILED:  { result = QString("failed");   break; }
        default:                     { Q_ASSERT(false);              break; }
    }

    return result;
}
