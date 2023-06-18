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
* This header implements the \ref Monitor class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QMap>
#include <QWeakPointer>
#include <QByteArray>
#include <QElapsedTimer>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslCertificate>

#include <cstdint>
#include <chrono>

#include <html_scrubber_hasher.h>

#include "customer.h"
#include "host_scheme.h"
#include "data_aggregator.h"
#include "http_service_thread.h"
#include "monitor.h"

const QByteArray Monitor::defaultUserAgent("InesonicBot");
const QByteArray Monitor::defaultUserAgentHeaderString("user-agent");
const QByteArray Monitor::textPlainContentType("text/plain");
const QByteArray Monitor::applicationJsonContentType("application/json");
const QByteArray Monitor::applicationXmlContentType("application/xml");
const QByteArray Monitor::optionsVerb("OPTIONS");
const QByteArray Monitor::patchVerb("PATCH");

QMap<QByteArray, QByteArray> Monitor::currentDefaultHeaders;

Monitor::Monitor(
        Monitor::MonitorId        monitorId,
        const QString&            path,
        Monitor::Method           method,
        Monitor::ContentCheckMode contentCheckMode,
        const KeywordList&        keywords,
        Monitor::ContentType      contentType,
        const QString&            userAgent,
        const QByteArray&         postContent,
        HostScheme*               hostScheme
    ):currentMonitorId(
        monitorId
    ),currentPath(
        path
    ),currentMethod(
        method
    ),currentContentCheckMode(
        contentCheckMode
    ),currentKeywords(
        keywords
    ),currentContentType(
        contentType
    ),currentUserAgent(
        userAgent.toUtf8()
    ),currentPostContent(
        postContent
    ) {
    currentMonitorStatus = MonitorStatus::UNKNOWN;
    pendingReply         = nullptr;

    if (hostScheme != nullptr) {
        moveToThread(hostScheme->thread());
        setParent(hostScheme);
        hostScheme->monitorAdded(this);
    }

    connect(this, &Monitor::startCheckRequested, this, &Monitor::startCheck);
}


Monitor::~Monitor() {}


Monitor::MonitorStatus Monitor::monitorStatus() const {
    return currentMonitorStatus;
}


Monitor::MonitorId Monitor::monitorId() const {
    return currentMonitorId;
}


HostScheme* Monitor::hostScheme() const {
    return static_cast<HostScheme*>(parent());
}


void Monitor::setHostScheme(HostScheme* newHostScheme) {
    newHostScheme->addMonitor(this);
}


const QString& Monitor::path() const {
    return currentPath;
}


void Monitor::setPath(const QString& newPath) {
    currentPath = newPath;
}


Monitor::Method Monitor::method() const {
    return currentMethod;
}


void Monitor::setMethod(Monitor::Method newMethod) {
    currentMethod = newMethod;
}


Monitor::ContentCheckMode Monitor::contentCheckMode() const {
    return currentContentCheckMode;
}


void Monitor::setContentCheckMode(Monitor::ContentCheckMode newContentCheckMode) {
    currentContentCheckMode = newContentCheckMode;
}


const Monitor::KeywordList& Monitor::keywords() const {
    return currentKeywords;
}


void Monitor::setKeywords(const Monitor::KeywordList& newKeywordList) {
    currentKeywords = newKeywordList;
}


Monitor::ContentType Monitor::contentType() const {
    return currentContentType;
}


void Monitor::setContentType(Monitor::ContentType newContentType) {
    currentContentType = newContentType;
}


QString Monitor::userAgent() const {
    return QString::fromUtf8(currentUserAgent);
}


void Monitor::setUserAgent(const QString& newUserAgent) {
    currentUserAgent = newUserAgent.toUtf8();
}


const QByteArray& Monitor::postContent() const {
    return currentPostContent;
}


void Monitor::setPostContent(const QByteArray& newPostContent) {
    currentPostContent = newPostContent;
}


QString Monitor::toString(Method method) {
    QString result;

    switch (method) {
        case Method::GET:     { result = QString("GET");      break; }
        case Method::HEAD:    { result = QString("HEAD");     break; }
        case Method::POST:    { result = QString("POST");     break; }
        case Method::PUT:     { result = QString("PUT");      break; }
        case Method::DELETE:  { result = QString("DELETE");   break; }
        case Method::OPTIONS: { result = QString("OPTIONS");  break; }
        case Method::PATCH:   { result = QString("PATCH");    break; }
        default:              { Q_ASSERT(false);              break; }
    }

    return result;
}


Monitor::Method Monitor::toMethod(const QString& str, bool* ok) {
    Method result;
    bool   success = true;

    QString l = str.trimmed().toLower();
    if (l == "get") {
        result = Method::GET;
    } else if (l == "head") {
        result = Method::HEAD;
    } else if (l == "post") {
        result = Method::POST;
    } else if (l == "put") {
        result = Method::PUT;
    } else if (l == "delete") {
        result = Method::DELETE;
    } else if (l == "options") {
        result = Method::OPTIONS;
    } else if (l == "patch") {
        result = Method::PATCH;
    } else {
        result  = Method::GET;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


QString Monitor::toString(Monitor::ContentCheckMode contentCheckMode) {
    QString result;

    switch (contentCheckMode) {
        case ContentCheckMode::NO_CHECK:            { result = QString("NO_CHECK");             break; }
        case ContentCheckMode::CONTENT_MATCH:       { result = QString("CONTENT_MATCH");        break; }
        case ContentCheckMode::ALL_KEYWORDS:        { result = QString("ALL_KEYWORDS");         break; }
        case ContentCheckMode::ANY_KEYWORDS:        { result = QString("ANY_KEYWORDS");         break; }
        case ContentCheckMode::SMART_CONTENT_MATCH: { result = QString("SMART_CONTENT_MATCH");  break; }
        default:                                    { Q_ASSERT(false);                          break; }
    }

    return result;
}


Monitor::ContentCheckMode Monitor::toContentCheckMode(const QString& str, bool* ok) {
    ContentCheckMode result;
    bool             success = true;

    QString l = str.trimmed().toLower().replace('-', '_');
    if (l == "no_check") {
        result = ContentCheckMode::NO_CHECK;
    } else if (l == "content_match") {
        result = ContentCheckMode::CONTENT_MATCH;
    } else if (l == "all_keywords") {
        result = ContentCheckMode::ALL_KEYWORDS;
    } else if (l == "any_keywords") {
        result = ContentCheckMode::ANY_KEYWORDS;
    } else if (l == "smart_content_match") {
        result = ContentCheckMode::SMART_CONTENT_MATCH;
    } else {
        result  = ContentCheckMode::NO_CHECK;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


QString Monitor::toString(Monitor::ContentType contentType) {
    QString result;

    switch (contentType) {
        case ContentType::JSON: { result = QString("JSON");   break; }
        case ContentType::TEXT: { result = QString("TEXT");   break; }
        case ContentType::XML:  { result = QString("XML");    break; }
        default:                { Q_ASSERT(false);            break; }
    }

    return result;
}


Monitor::ContentType Monitor::toContentType(const QString& str, bool* ok) {
    ContentType result;
    bool        success = true;

    QString l = str.trimmed().toLower();
    if (l == "json") {
        result = ContentType::JSON;
    } else if (l == "text") {
        result = ContentType::TEXT;
    } else if (l == "xml") {
        result = ContentType::XML;
    } else {
        result  = ContentType::TEXT;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


void Monitor::startCheckFromDifferentThread() {
    emit startCheckRequested();
}


void Monitor::setDefaultHeaders(const Headers& headers) {
    currentDefaultHeaders.clear();

    for (Headers::const_iterator it=headers.constBegin(),end=headers.constEnd() ; it!=end ; ++it) {
        currentDefaultHeaders.insert(it.key().toLatin1(), it.value().toLatin1());
    }
}


Monitor::Headers Monitor::defaultHeaders() {
    Headers result;
    for (  QMap<QByteArray, QByteArray>:: const_iterator it  = currentDefaultHeaders.constBegin(),
                                                         end = currentDefaultHeaders.constEnd()
         ; it != end
         ; ++it
        ) {
        result.insert(QString::fromLatin1(it.key()), QString::fromLatin1(it.value()));
    }

    return result;
}


void Monitor::startCheck() {
    if (pendingReply == nullptr) {
        HostScheme* hostScheme = static_cast<HostScheme*>(parent());
        if (hostScheme != nullptr) {
            Customer* customer = static_cast<Customer*>(hostScheme->parent());
            if (!customer->paused()) {
                QUrl url = hostScheme->url();
                url.setPath(currentPath);

                if (currentMethod == Method::GET     ||
                    currentMethod == Method::HEAD    ||
                    currentMethod == Method::DELETE  ||
                    currentMethod == Method::OPTIONS    ) {
                    QNetworkRequest request(url);

                    const QByteArray* userAgent = &defaultUserAgent;

                    for (  QMap<QByteArray, QByteArray>::const_iterator it  = currentDefaultHeaders.constBegin(),
                                                                        end = currentDefaultHeaders.constEnd()
                         ; it != end
                         ; ++it
                        ) {
                        if (it.key() == defaultUserAgentHeaderString) {
                            userAgent = &it.value();
                        } else {
                            request.setRawHeader(it.key(), it.value());
                        }
                    }

                    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, *userAgent);
                    request.setHeader(QNetworkRequest::KnownHeaders::ContentLengthHeader, 0);

                    startTimestamp = QDateTime::currentSecsSinceEpoch();
                    elapsedTimer.start();
                    if (currentMethod == Method::GET) {
                        pendingReply = hostScheme->networkAccessManager()->get(request);
                    } else if (currentMethod == Method::HEAD) {
                        pendingReply = hostScheme->networkAccessManager()->head(request);
                    } else if (currentMethod == Method::DELETE) {
                        pendingReply = hostScheme->networkAccessManager()->deleteResource(request);
                    } else {
                        Q_ASSERT(currentMethod == Method::OPTIONS);
                        pendingReply = hostScheme->networkAccessManager()->sendCustomRequest(request, optionsVerb);
                    }
                } else {
                    Q_ASSERT(
                           currentMethod == Method::POST
                        || currentMethod == Method::PUT
                        || currentMethod == Method::PATCH
                    );

                    QNetworkRequest request(url);

                    const QByteArray* userAgent = &defaultUserAgent;
                    for (  QMap<QByteArray, QByteArray>::const_iterator it  = currentDefaultHeaders.constBegin(),
                                                                        end = currentDefaultHeaders.constEnd()
                         ; it != end
                         ; ++it
                        ) {
                        if (it.key() == defaultUserAgentHeaderString) {
                            userAgent = &it.value();
                        } else {
                            request.setRawHeader(it.key(), it.value());
                        }
                    }

                    if (currentUserAgent.isEmpty()) {
                        request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, *userAgent);
                    } else {
                        request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, currentUserAgent);
                    }

                    const QByteArray* contentType = nullptr;
                    switch (currentContentType) {
                        case ContentType::TEXT: { contentType = &textPlainContentType;        break; }
                        case ContentType::JSON: { contentType = &applicationJsonContentType;  break; }
                        case ContentType::XML:  { contentType = &applicationXmlContentType;   break; }
                        default:                { Q_ASSERT(false);                            break; }
                    }

                    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, *contentType);
                    request.setHeader(QNetworkRequest::KnownHeaders::ContentLengthHeader, currentPostContent.size());
                    request.setTransferTimeout(transferTimeout);

                    startTimestamp = QDateTime::currentSecsSinceEpoch();
                    elapsedTimer.start();
                    if (currentMethod == Method::POST) {
                        pendingReply = hostScheme->networkAccessManager()->post(request, currentPostContent);
                    } else if (currentMethod == Method::PUT) {
                        pendingReply = hostScheme->networkAccessManager()->put(request, currentPostContent);
                    } else {
                        Q_ASSERT(currentMethod == Method::PATCH);
                        pendingReply = hostScheme->networkAccessManager()->sendCustomRequest(
                            request,
                            patchVerb,
                            currentPostContent
                        );
                    }
                }

                pendingReply->setParent(this);
                connect(pendingReply, &QNetworkReply::finished, this, &Monitor::responseReceived);
            }
        } else {
            lastHash.clear();
        }
    }
}


void Monitor::abort() {
    if (pendingReply != nullptr) {
        delete pendingReply;
    }

    currentMonitorStatus = MonitorStatus::UNKNOWN;
}


void Monitor::responseReceived() {
    unsigned long long          elapsedNanoseconds = elapsedTimer.nsecsElapsed();
    QNetworkReply::NetworkError networkError       = pendingReply->error();

    pendingReply->deleteLater();

    if (networkError == QNetworkReply::NetworkError::NoError) {
        QSslConfiguration sslConfiguration = pendingReply->sslConfiguration();
        processValidResponse(elapsedNanoseconds, sslConfiguration);
    } else {
        processErrorResponse();
    }

    pendingReply = nullptr;
}


void Monitor::processValidResponse(
        unsigned long long       elapsedTimeNanoseconds,
        const QSslConfiguration& sslConfiguration
    ) {
    HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
    DataAggregator*    dataAggregator = serviceThread->dataAggregator();

    if (currentMonitorStatus != MonitorStatus::WORKING) {
        hostScheme()->monitorNowResponsive(this);
        dataAggregator->reportEvent(
            currentMonitorId,
            QDateTime::currentSecsSinceEpoch(),
            DataAggregator::EventType::WORKING,
            currentMonitorStatus,
            QByteArray()
        );
    }

    currentMonitorStatus = MonitorStatus::WORKING;

    if (currentContentCheckMode != ContentCheckMode::NO_CHECK) {
        QByteArray receivedData = pendingReply->readAll();

        if (currentContentCheckMode == ContentCheckMode::CONTENT_MATCH) {
            checkContentChange(receivedData);
        } else if (currentContentCheckMode == ContentCheckMode::ANY_KEYWORDS) {
            checkAnyKeywordMatch(receivedData);
        } else if (currentContentCheckMode == ContentCheckMode::ALL_KEYWORDS) {
            checkAllKeywordMatch(receivedData);
        } else if (currentContentCheckMode == ContentCheckMode::SMART_CONTENT_MATCH) {
            checkContentChangeSmart(receivedData);
        } else {
            Q_ASSERT(false); // Unexpected content check mode.
        }
    }

    Customer* customer = hostScheme()->customer();
    if (customer->supportsLatencyMeasurements()) {
        unsigned long elapsedTimeMicroseconds = static_cast<unsigned long>((elapsedTimeNanoseconds + 500) / 1000);
        if (elapsedTimeMicroseconds <= maximumAllowedLatencyMicroseconds) {
            dataAggregator->recordLatency(currentMonitorId, startTimestamp, elapsedTimeMicroseconds);
        }
    }

    if (!sslConfiguration.isNull()) {
        QSslCertificate   certificate = sslConfiguration.peerCertificate();

        if (!certificate.isNull()) {
            QDateTime certificateExpirationDateTime = certificate.expiryDate();

            HostScheme* hostScheme = static_cast<HostScheme*>(parent());
            if (hostScheme != nullptr) {
                unsigned long long newExpirationTimestamp = certificateExpirationDateTime.toSecsSinceEpoch();
                unsigned long long oldExpirationTimestamp = hostScheme->sslExpirationTimestamp();

                if (oldExpirationTimestamp != newExpirationTimestamp) {
                    hostScheme->setSslExpirationTimestamp(newExpirationTimestamp);
                    dataAggregator->reportSslCertificateExpirationChange(
                        currentMonitorId,
                        hostScheme->hostSchemeId(),
                        newExpirationTimestamp
                    );
                }
            }
        }
    }
}

void Monitor::processErrorResponse() {
    if (currentMonitorStatus != MonitorStatus::FAILED) {
        QString            errorMessage   = pendingReply->errorString();

        HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
        DataAggregator*    dataAggregator = serviceThread->dataAggregator();

        dataAggregator->reportEvent(
            currentMonitorId,
            QDateTime::currentSecsSinceEpoch(),
            DataAggregator::EventType::NO_RESPONSE,
            currentMonitorStatus,
            QByteArray(),
            errorMessage
        );

        currentMonitorStatus = MonitorStatus::FAILED;

        hostScheme()->monitorNonResponsive(this);
    }
}


void Monitor::checkContentChange(const QByteArray& receivedData) {
    QCryptographicHash hash(QCryptographicHash::Algorithm::Sha256);
    hash.addData(reinterpret_cast<const char*>(&currentMonitorId), sizeof(MonitorId));
    hash.addData(receivedData);

    if (lastHash.isEmpty()) {
        lastHash = hash.result();
    } else {
        QByteArray thisHash = hash.result();
        if (lastHash != thisHash) {
            HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
            DataAggregator*    dataAggregator = serviceThread->dataAggregator();

            dataAggregator->reportEvent(
                currentMonitorId,
                QDateTime::currentSecsSinceEpoch(),
                DataAggregator::EventType::CONTENT_CHANGED,
                currentMonitorStatus,
                thisHash
            );

            lastHash = thisHash;
        }
    }
}


void Monitor::checkAnyKeywordMatch(const QByteArray& receivedData) {
    unsigned numberKeywords = static_cast<unsigned>(currentKeywords.size());
    if (numberKeywords > 0) {
        QCryptographicHash hash(QCryptographicHash::Algorithm::Sha256);
        hash.addData(reinterpret_cast<const char*>(&currentMonitorId), sizeof(MonitorId));
        hash.addData(receivedData);

        bool     success = false;
        unsigned i       = 0;
        do {
            const QByteArray& keyword = currentKeywords.at(i);
            success = receivedData.contains(keyword);
            if (success) {
                hash.addData(keyword);
            }
            ++i;
        } while (!success && i < numberKeywords);

        QByteArray thisHash = hash.result();

        if (!success && lastHash != thisHash) {
            HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
            DataAggregator*    dataAggregator = serviceThread->dataAggregator();

            dataAggregator->reportEvent(
                currentMonitorId,
                QDateTime::currentSecsSinceEpoch(),
                DataAggregator::EventType::KEYWORDS,
                currentMonitorStatus,
                thisHash
            );
        }

        lastHash = thisHash;
    }
}


void Monitor::checkAllKeywordMatch(const QByteArray& receivedData) {
    unsigned numberKeywords = static_cast<unsigned>(currentKeywords.size());
    if (numberKeywords > 0) {
        QCryptographicHash hash(QCryptographicHash::Algorithm::Sha256);
        hash.addData(reinterpret_cast<const char*>(&currentMonitorId), sizeof(MonitorId));
        hash.addData(receivedData);

        bool     success = true;
        QString  missingKeyword;
        unsigned i       = 0;
        do {
            const QByteArray& keyword = currentKeywords.at(i);
            success = receivedData.contains(keyword);
            if (success) {
                hash.addData(keyword);
                ++i;
            } else {
                missingKeyword = QString::fromUtf8(currentKeywords.at(i));
            }
        } while (success && i < numberKeywords);

        QByteArray thisHash = hash.result();

        if (!success && lastHash != thisHash) {
            HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
            DataAggregator*    dataAggregator = serviceThread->dataAggregator();

            dataAggregator->reportEvent(
                currentMonitorId,
                QDateTime::currentSecsSinceEpoch(),
                DataAggregator::EventType::KEYWORDS,
                currentMonitorStatus,
                thisHash,
                QString("Missing keyword \"%1\"").arg(missingKeyword)
            );
        }

        lastHash = thisHash;
    }
}


void Monitor::checkContentChangeSmart(const QByteArray& receivedData) {
    HtmlScrubber::Hasher hasher(receivedData, HtmlScrubber::Hasher::Algorithm::Sha256);
    hasher.scrubAndHash();
    hasher.addData(reinterpret_cast<const char*>(&currentMonitorId), sizeof(MonitorId));

    QByteArray hash = hasher.result();

    if (lastHash.isEmpty()) {
        lastHash = hash;
    } else {
        if (lastHash != hash) {
            HttpServiceThread* serviceThread  = static_cast<HttpServiceThread*>(thread());
            DataAggregator*    dataAggregator = serviceThread->dataAggregator();

            dataAggregator->reportEvent(
                currentMonitorId,
                QDateTime::currentSecsSinceEpoch(),
                DataAggregator::EventType::CONTENT_CHANGED,
                currentMonitorStatus,
                hash
            );

            lastHash = hash;
        }
    }
}
