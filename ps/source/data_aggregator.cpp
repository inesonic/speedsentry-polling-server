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
* This header implements the \ref DataAggregator class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_binary_rest_handler.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

#include <cstdint>
#include <cstring>

#include "log.h"
#include "metatypes.h"
#include "resources.h"
#include "service_thread_tracker.h"
#include "event_reporter.h"
#include "certificate_reporter.h"
#include "data_aggregator.h"

const QString  DataAggregator::latencyRecordPath("/latency/record");

DataAggregator::DataAggregator(
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicBinaryRestHandler(
        server,
        parent
    ),currentServiceThreadTracker(
        nullptr
    ) {
    configure();
}


DataAggregator::DataAggregator(
        const QByteArray&     secret,
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicBinaryRestHandler(
        secret,
        server,
        parent
    ),currentServiceThreadTracker(
        nullptr
    ) {
    configure();
}


DataAggregator::DataAggregator(
        ServiceThreadTracker* serviceThreadTracker,
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicBinaryRestHandler(
        server,
        parent
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {
    configure();
}


DataAggregator::DataAggregator(
        ServiceThreadTracker* serviceThreadTracker,
        const QByteArray&     secret,
        RestApiOutV1::Server* server,
        QObject*              parent
    ):RestApiOutV1::InesonicBinaryRestHandler(
        secret,
        server,
        parent
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {
    configure();
}


DataAggregator::~DataAggregator() {
    delete latencyEntryList;
}


ServiceThreadTracker* DataAggregator::serviceThreadTracker() const {
    return currentServiceThreadTracker;
}


void DataAggregator::setServiceThreadTracker(ServiceThreadTracker* newServiceThreadTracker) {
    currentServiceThreadTracker = newServiceThreadTracker;
}


QString DataAggregator::serverIdentifier() const {
    return QString::fromUtf8(
        reinterpret_cast<const char*>(headerTemplate.identifier),
        maximumIdentifierLength
    );
}


void DataAggregator::setServerIdentifier(const QString& newServerIdentifier) {
    QByteArray rawData       = newServerIdentifier.toUtf8();
    unsigned   rawDataLength = static_cast<unsigned>(rawData.size());
    unsigned   bytesToCopy   = rawDataLength > maximumIdentifierLength ? maximumIdentifierLength : rawDataLength;

    std::memcpy(headerTemplate.identifier, rawData.constData(), bytesToCopy);
    if (bytesToCopy < maximumIdentifierLength) {
        std::memset(headerTemplate.identifier + bytesToCopy, 0, maximumIdentifierLength - bytesToCopy);
    }
}


void DataAggregator::recordLatency(
        DataAggregator::MonitorId           monitorId,
        unsigned long long                  timestamp,
        DataAggregator::LatencyMicroseconds latency
    ) {
    QMutexLocker locker(&listMutex);
    latencyEntryList->append(LatencyEntry(monitorId, timestamp, latency));

    if (inFlightLatencyEntryList == nullptr) {
        if (static_cast<unsigned>(latencyEntryList->size()) >= maximumNumberPendingEntries) {
            emit triggerReporting();
        } else if (!reportTimer->isActive()) {
            emit triggerReporting(maximumReportDelayMilliseconds);
        }
    }
}


void DataAggregator::reportEvent(
        DataAggregator::MonitorId monitorId,
        unsigned long long        timestamp,
        DataAggregator::EventType eventType,
        Monitor::MonitorStatus    monitorStatus,
        const QByteArray&         hash,
        const QString&            message
    ) {
    emit reportEventRequested(
        monitorId,
        timestamp,
        static_cast<unsigned>(eventType),
        static_cast<unsigned>(monitorStatus),
        hash,
        message
    );
}


void DataAggregator::reportSslCertificateExpirationChange(
        DataAggregator::MonitorId    monitorId,
        DataAggregator::HostSchemeId hostSchemeId,
        unsigned long long           newExpirationTimestamp
    ) {
    emit reportSslCertificateExpirationChangeRequested(monitorId, hostSchemeId, newExpirationTimestamp);
}


void DataAggregator::sendReport() {
    emit triggerReporting();
}


void DataAggregator::processReportEvent(
        unsigned long      monitorId,
        unsigned long long timestamp,
        unsigned           eventType,
        unsigned           monitorStatus,
        const QByteArray&  hash,
        const QString&     message
    ) {
    eventReporter->sendEvent(
        monitorId,
        timestamp,
        static_cast<EventType>(eventType),
        static_cast<MonitorStatus>(monitorStatus),
        hash,
        message
    );
}


void DataAggregator::processReportSslCertificateExpirationChange(
        unsigned long      monitorId,
        unsigned long      hostSchemeId,
        unsigned long long newExpirationTimestamp
    ) {
    CertificateReporter* certificateReporter = new CertificateReporter(server(), this);
    certificateReporter->startReporting(monitorId, hostSchemeId, newExpirationTimestamp);
    // Note: certificateReporter calls deleteLater on itself on successful completion... fire and forget.
}


void DataAggregator::processResponse(const QByteArray& binaryData, const QString& contentType) {
    if (contentType == QString("application/json")) {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(binaryData);
        if (jsonDocument.isObject()) {
            QJsonObject responseObject = jsonDocument.object();
            QString     status         = responseObject.value("status").toString();
            if (status == QString("OK")) {
                unsigned long numberEntries = static_cast<unsigned long>(inFlightLatencyEntryList->size());
                if (numberEntries > 0) {
                    logWrite(
                        QString("Sent %1 latency entries for timestamps %2-%3.")
                        .arg(inFlightLatencyEntryList->size())
                        .arg(inFlightLatencyEntryList->first().unixTimestamp())
                        .arg(inFlightLatencyEntryList->last().unixTimestamp()),
                        false
                    );
                } else {
                    logWrite(QString("Sent empty latency entry report."), false);
                }

                delete inFlightLatencyEntryList;

                QMutexLocker locker(&listMutex);
                inFlightLatencyEntryList = nullptr;

                unsigned long currentNumberEntries = static_cast<unsigned>(latencyEntryList->size());
                if (currentNumberEntries >= maximumNumberPendingEntries) {
                    emit triggerReporting();
                } else if (currentNumberEntries > 0) {
                    emit triggerReporting(maximumReportDelayMilliseconds);
                }
            } else {
                processRequestFailed(QString("Database controller reported \"%1\"").arg(status));
            }
        } else {
            processRequestFailed(QString("Expected JSON object."));
        }
    } else {
        processRequestFailed(QString("Unexpected response content type."));
    }
}


void DataAggregator::processRequestFailed(const QString& errorString) {
    logWrite(
        QString("Latency report failed: %1 -- retrying in %2 seconds.")
        .arg(errorString)
        .arg(retryDelaySeconds),
        false
    );

    emit triggerRetry(1000 * retryDelaySeconds);
}


void DataAggregator::startReportingLatencyData() {
    listMutex.lock();
    if (inFlightLatencyEntryList == nullptr) {
        inFlightLatencyEntryList = latencyEntryList;

        latencyEntryList = new LatencyEntryList;
        latencyEntryList->reserve(inFlightLatencyEntryList->size());

        listMutex.unlock();

        sendReport(*inFlightLatencyEntryList);
    } else {
        listMutex.unlock();
    }
}


void DataAggregator::startRetry() {
    if (inFlightLatencyEntryList != nullptr) {
        sendReport(*inFlightLatencyEntryList);
    }
}


void DataAggregator::configure() {
    latencyEntryList         = new LatencyEntryList();
    inFlightLatencyEntryList = nullptr;
    reportTimer              = new QTimer(this);
    retryTimer               = new QTimer(this);

    eventReporter            = new EventReporter(server(), this);

    std::memset(&headerTemplate, 0, sizeof(Header));
    headerTemplate.version = 0; // To be explicit.

    reportTimer->setSingleShot(true);
    connect(reportTimer, &QTimer::timeout, this, &DataAggregator::startReportingLatencyData);
    connect(
        this,
        &DataAggregator::triggerReporting,
        reportTimer,
        static_cast<void (QTimer::*)(int)>(&QTimer::start)
    );

    retryTimer->setSingleShot(true);
    connect(retryTimer, &QTimer::timeout, this, &DataAggregator::startRetry);
    connect(this, &DataAggregator::triggerRetry, retryTimer, static_cast<void (QTimer::*)(int)>(&QTimer::start));

    connect(this, &DataAggregator::reportEventRequested, this, &DataAggregator::processReportEvent);
    connect(
        this,
        &DataAggregator::reportSslCertificateExpirationChangeRequested,
        this,
        &DataAggregator::processReportSslCertificateExpirationChange
    );
}


void DataAggregator::sendReport(const DataAggregator::LatencyEntryList& latencyEntryList) {
    unsigned long numberLatencyEntries = static_cast<unsigned long>(latencyEntryList.size());
    QByteArray    message(sizeof(Header) + numberLatencyEntries * sizeof(Entry), '\x00');

    std::uint8_t* messageData = reinterpret_cast<std::uint8_t*>(message.data());
    Header*       header      = reinterpret_cast<Header*>(messageData);
    Entry*        entry       = reinterpret_cast<Entry*>(messageData + sizeof(Header));

    memcpy(header, &headerTemplate, sizeof(Header));
    header->monitorsPerSecond = static_cast<std::uint32_t>(currentServiceThreadTracker->monitorsPerSecond() * 256.0);
    header->cpuLoading        = std::min(65535U, static_cast<unsigned>(cpuUtilization() * 4096.0));
    header->memoryLoading     = std::min(65535U, static_cast<unsigned>(memoryUtilization() * 65536.0));
    header->serverStatusCode  = static_cast<std::uint8_t>(currentServiceThreadTracker->status());

    for (unsigned long i=0 ; i<numberLatencyEntries ; ++i) {
        const LatencyEntry& latencyEntry = latencyEntryList.at(i);
        entry->monitorId           = latencyEntry.monitorId();
        entry->timestamp           = latencyEntry.zoranTimestamp();
        entry->latencyMicroseconds = latencyEntry.latencyMicroseconds();

        ++entry;
    }

    post(latencyRecordPath, message);
}
