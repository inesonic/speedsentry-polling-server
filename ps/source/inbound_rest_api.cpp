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
* This header implements the \ref InboundRestApi class.
***********************************************************************************************************************/

#include <QObject>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "service_thread_tracker.h"
#include "resources.h"
#include "loading_data.h"
#include "inbound_rest_api.h"

/***********************************************************************************************************************
* InboundRestApi::StateActive
*/

InboundRestApi::StateActive::StateActive(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::StateActive::~StateActive() {}


RestApiInV1::JsonResponse InboundRestApi::StateActive::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& /* request */,
        unsigned             /* threadId */
    ) {
    currentServiceThreadTracker->goActive();

    QJsonObject responseObject;
    responseObject.insert("status", "OK");

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* InboundRestApi::StateInactive
*/

InboundRestApi::StateInactive::StateInactive(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::StateInactive::~StateInactive() {}


RestApiInV1::JsonResponse InboundRestApi::StateInactive::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& /* request */,
        unsigned             /* threadId */
    ) {
    currentServiceThreadTracker->goInactive();

    QJsonObject responseObject;
    responseObject.insert("status", "OK");

    return RestApiInV1::JsonResponse(responseObject);
}

/***********************************************************************************************************************
* InboundRestApi::RegionChange
*/

InboundRestApi::RegionChange::RegionChange(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::RegionChange::~RegionChange() {}


RestApiInV1::JsonResponse InboundRestApi::RegionChange::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("region_index") && requestObject.contains("number_regions")) {
            QJsonObject responseObject;

            int regionIndex   = requestObject.value("region_index").toInt(-1);
            int numberRegions = requestObject.value("number_regions").toInt(-1);

            if (numberRegions > 0 && regionIndex >= 0 && regionIndex < numberRegions) {
                currentServiceThreadTracker->updateRegionData(
                    static_cast<unsigned>(regionIndex),
                    static_cast<unsigned>(numberRegions)
                );

                responseObject.insert("status", "OK");
            } else {
                responseObject.insert("status", "failed, invalid parameters");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* InboundRestApi::LoadingGet
*/

InboundRestApi::LoadingGet::LoadingGet(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::LoadingGet::~LoadingGet() {}


RestApiInV1::JsonResponse InboundRestApi::LoadingGet::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& /* request */,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    QJsonObject responseObject;

    responseObject.insert("status", "OK");

    QJsonObject loadingObject;
    loadingObject.insert("cpu", cpuUtilization());
    loadingObject.insert("memory", memoryUtilization());

    QMultiMap<int, LoadingData>        loadingData = currentServiceThreadTracker->loadingData();
    QMap<unsigned, QList<LoadingData>> singleRegionLoadingData;
    QMap<unsigned, QList<LoadingData>> multiRegionLoadingData;
    for ( QMultiMap<int, LoadingData>::const_iterator loadingDataIterator    = loadingData.constBegin(),
                                                      loadingDataEndIterator = loadingData.constEnd()
         ; loadingDataIterator != loadingDataEndIterator
         ; ++loadingDataIterator
        ) {
        int                pollingInterval = loadingDataIterator.key();
        const LoadingData& loadingData     = loadingDataEndIterator.value();

        if (pollingInterval < 0) {
            singleRegionLoadingData[static_cast<unsigned>(-pollingInterval)].append(loadingData);
        } else {
            multiRegionLoadingData[static_cast<unsigned>(+pollingInterval)].append(loadingData);
        }
    }

    loadingObject.insert("single_region", generateResponse(singleRegionLoadingData));
    loadingObject.insert("multi_region", generateResponse(multiRegionLoadingData));

    responseObject.insert("data", loadingObject);

    return RestApiInV1::JsonResponse(responseObject);
}


QJsonObject InboundRestApi::LoadingGet::generateResponse(const QMap<unsigned, QList<LoadingData>>& loadingData) {
    QJsonObject result;

    for ( QMap<unsigned, QList<LoadingData>>::const_iterator pit  = loadingData.constBegin(),
                                                             pend = loadingData.constEnd()
         ; pit != pend
         ; ++pit
        ) {
        unsigned                  pollingInterval = pit.key();
        const QList<LoadingData>& loadingDataList = pit.value();
        QJsonArray                array;

        for (  QList<LoadingData>::const_iterator lit = loadingDataList.constBegin(), lend = loadingDataList.constEnd()
             ; lit != lend
             ; ++lit
            ) {
            const LoadingData& data = *lit;

            QJsonObject loadingDataObject;
            loadingDataObject.insert("polled_host_schemes", static_cast<double>(data.numberPolledHostSchemes()));
            loadingDataObject.insert("missed_timing_marks", static_cast<double>(data.numberMissedTimingMarks()));
            loadingDataObject.insert("average_timing_error", data.averageTimingError());

            array.append(loadingDataObject);
        }

        result.insert(QString::number(pollingInterval), array);
    }

    return result;
}

/***********************************************************************************************************************
* InboundRestApi::CustomerAdd
*/

InboundRestApi::CustomerAdd::CustomerAdd(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::CustomerAdd::~CustomerAdd() {}


RestApiInV1::JsonResponse InboundRestApi::CustomerAdd::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QString     statusString("OK");
        bool        success       = true;
        QJsonObject requestObject = request.object();

        QJsonObject::const_iterator customerIterator    = requestObject.constBegin();
        QJsonObject::const_iterator customerEndIterator = requestObject.constEnd();

        QList<Customer*> customers;
        while (success && customerIterator != customerEndIterator) {
            QString    customerIdString  = customerIterator.key();
            QJsonValue customerDataValue = customerIterator.value();

            Customer::CustomerId customerId = customerIdString.toUInt(&success);
            if (success && customerId != 0) {
                if (customerDataValue.isObject()) {
                    Customer* customer = generateCustomer(
                        success,
                        statusString,
                        customerId,
                        customerDataValue.toObject()
                    );

                    if (customer != nullptr) {
                        customers.append(customer);
                    }
                } else {
                    success      = false;
                    statusString = QString("Expected object for customer %1").arg(customerId);
                }
            } else {
                success      = false;
                statusString = QString("Invalid customer ID %1").arg(customerIdString);
            }

            ++customerIterator;
        }

        if (success) {
            for (  QList<Customer*>::const_iterator customerIterator = customers.constBegin(),
                                                    customerEndIterator = customers.constEnd()
                 ; customerIterator != customerEndIterator
                 ; ++customerIterator
                ) {
                Customer* customer = *customerIterator;

                currentServiceThreadTracker->removeCustomer(customer->customerId());
                currentServiceThreadTracker->addCustomer(customer);
            }
        } else {
            for (QList<Customer*>::const_iterator it=customers.constBegin(),end=customers.constEnd() ; it!=end ; ++it) {
                delete *it;
            }
        }

        QJsonObject responseObject;
        responseObject.insert("status", statusString);
        response = RestApiInV1::JsonResponse(responseObject);
    }

    return response;
}


Customer* InboundRestApi::CustomerAdd::generateCustomer(
        bool&                success,
        QString&             statusString,
        Customer::CustomerId customerId,
        const QJsonObject&   jsonObject
    ) {
    Customer* result = nullptr;

    if (jsonObject.contains("polling_interval") && jsonObject.contains("host_schemes")) {
        QJsonValue hostSchemesValue = jsonObject.value("host_schemes");
        if (hostSchemesValue.isObject()) {
            int pollingIntervalInt = jsonObject.value("polling_interval").toInt(-1);

            if (pollingIntervalInt >= 20) {
                unsigned pollingInterval              = static_cast<unsigned>(pollingIntervalInt);
                bool     supportsPingTesting          = jsonObject.value("ping").toBool(false);
                bool     supportsSslExpirationTesting = jsonObject.value("ssl_expiration").toBool(false);
                bool     supportsLatencyMeasurements  = jsonObject.value("latency").toBool(false);
                bool     supportsMultiRegionTesting   = jsonObject.value("multi_region").toBool(false);

                result = new Customer(
                    customerId,
                    supportsPingTesting,
                    supportsSslExpirationTesting,
                    supportsLatencyMeasurements,
                    supportsMultiRegionTesting,
                    pollingInterval
                );

                QJsonObject                 hostSchemesObject      = hostSchemesValue.toObject();
                QJsonObject::const_iterator hostSchemesIterator    = hostSchemesObject.constBegin();
                QJsonObject::const_iterator hostSchemesEndIterator = hostSchemesObject.constEnd();
                while (success && hostSchemesIterator != hostSchemesEndIterator) {
                    QString  hostSchemeIdString = hostSchemesIterator.key();
                    unsigned hostSchemeId       = hostSchemeIdString.toUInt(&success);
                    if (success && hostSchemeId != 0) {
                        const QJsonValue& hostSchemeValue = hostSchemesIterator.value();
                        if (hostSchemeValue.isObject()) {
                            HostScheme* hostScheme = generateHostScheme(
                                success,
                                statusString,
                                hostSchemeId,
                                hostSchemeValue.toObject()
                            );

                            if (hostScheme != nullptr) {
                                result->addHostScheme(hostScheme);
                            }
                        } else {
                            success      = false;
                            statusString = QString("failed, expected object, host/scheme ID %1")
                                           .arg(hostSchemeId);
                        }
                    } else {
                        success      = false;
                        statusString = QString("failed, invalid host/scheme ID %1").arg(hostSchemeIdString);
                    }

                    ++hostSchemesIterator;
                }
            } else {
                success      = false;
                statusString = QString("failed, invalid polling interval, customer %1")
                               .arg(customerId);
            }
        } else {
            success      = false;
            statusString = QString("failed, expected host/schemes object, customer %1").arg(customerId);
        }
    } else {
        success      = false;
        statusString = QString("failed, missing required fields, customer %1").arg(customerId);
    }

    return result;
}


HostScheme* InboundRestApi::CustomerAdd::generateHostScheme(
        bool&                    success,
        QString&                 statusString,
        HostScheme::HostSchemeId hostSchemeId,
        const QJsonObject&       jsonData
    ) {
    HostScheme* result = nullptr;

    if (jsonData.contains("url") && jsonData.contains("monitors")) {
        QJsonValue monitorsValue = jsonData.value("monitors");
        if (monitorsValue.isObject()) {
            QString urlString = jsonData.value("url").toString();
            QUrl    url(urlString);

            if (url.isValid()) {
                result = new HostScheme(hostSchemeId, url);

                QJsonObject                 monitorsObject = monitorsValue.toObject();
                QJsonObject::const_iterator monitorsIterator    = monitorsObject.constBegin();
                QJsonObject::const_iterator monitorsEndIterator = monitorsObject.constEnd();
                while (success && monitorsIterator != monitorsEndIterator) {
                    QString            monitorIdString = monitorsIterator.key();
                    Monitor::MonitorId monitorId       = monitorIdString.toUInt(&success);
                    if (success && monitorId != 0) {
                        const QJsonValue& monitorValue = monitorsIterator.value();
                        if (monitorValue.isObject()) {
                            Monitor* monitor = generateMonitor(
                                success,
                                statusString,
                                monitorId,
                                monitorValue.toObject()
                            );

                            if (monitor != nullptr) {
                                result->addMonitor(monitor);
                            }
                        } else {
                            success      = false;
                            statusString = QString("failed, expected object, monitor ID %1").arg(monitorId);
                        }
                    } else {
                        success      = false;
                        statusString = QString("failed, invalid monitors ID %1").arg(monitorIdString);
                    }

                    ++monitorsIterator;
                }
            } else {
                success      = false;
                statusString = QString("failed, invalid URL, host/scheme %1").arg(hostSchemeId);
            }
        } else {
            success      = false;
            statusString = QString("failed, expected object, host/scheme %1").arg(hostSchemeId);
        }
    } else {
        success      = false;
        statusString = QString("failed, missing required fields, host/scheme %1").arg(hostSchemeId);
    }

    return result;
}


Monitor* InboundRestApi::CustomerAdd::generateMonitor(
        bool&              success,
        QString&           statusString,
        Monitor::MonitorId monitorId,
        const QJsonObject& jsonData
    ) {
    Monitor* result = nullptr;

    unsigned                  numberFields     = 0;
    Monitor::Method           method           = Monitor::Method::GET;
    Monitor::ContentCheckMode contentCheckMode = Monitor::ContentCheckMode::NO_CHECK;
    Monitor::ContentType      contentType      = Monitor::ContentType::TEXT;
    QString                   uriString;
    Monitor::KeywordList      keywords;
    QString                   userAgent;
    QByteArray                postContent;

    if (jsonData.contains("uri")) {
        QJsonValue uriValue = jsonData.value("uri");
        if (uriValue.isString()) {
            uriString = uriValue.toString();
        } else {
            success      = false;
            statusString = QString("failed, uri must be a string, monitor %1").arg(monitorId);
        }

        ++numberFields;
    } else {
        success      = false;
        statusString = QString("missing required field \"uri\", monitor ID %1").arg(monitorId);
    }

    if (jsonData.contains("method")) {
        QJsonValue methodValue = jsonData.value("method");
        if (methodValue.isString()) {
            method = Monitor::toMethod(methodValue.toString(), &success);
            if (!success) {
                statusString = QString("failed, invalid method, use \"get\" or \"post\", monitor ID %1")
                               .arg(monitorId);
            }
        } else {
            success      = false;
            statusString = QString("failed, method fields must be a string, monitor ID %1").arg(monitorId);
        }

        ++numberFields;
    }

    if (jsonData.contains("content_check_mode")) {
        QJsonValue contentCheckModeValue = jsonData.value("content_check_mode");
        if (contentCheckModeValue.isString()) {
            contentCheckMode = Monitor::toContentCheckMode(contentCheckModeValue.toString(), &success);
            if (!success) {
                statusString = QString(
                    "failed, invalid content_check_mode, use \"no_check\", \"content_match\", \"all_keywords\", or "
                    "\"any_keywords\", monitor ID %1"
                ).arg(monitorId);
            }
        } else {
            success      = false;
            statusString = QString("failed, content_check_mode fields must be a string, monitor ID %1").arg(monitorId);
        }

        ++numberFields;
    }

    if (jsonData.contains("post_content_type")) {
        QJsonValue contentTypeValue = jsonData.value("post_content_type");
        if (contentTypeValue.isString()) {
            contentType = Monitor::toContentType(contentTypeValue.toString(), &success);
            if (!success) {
                statusString = QString(
                    "failed, invalid post_content_type, use \"text\", \"json\", or \"xml\", monitor ID %1"
                ).arg(monitorId);
            }
        } else {
            success      = false;
            statusString = QString("failed, post_content_type fields must be a string, monitor ID %1").arg(monitorId);
        }

        ++numberFields;
    }

    if (jsonData.contains("keywords")) {
        QJsonValue keywordsValue = jsonData.value("keywords");
        success = keywordsValue.isArray();
        if (success) {
            QJsonArray keywordsArray = keywordsValue.toArray();
            keywords.reserve(keywordsArray.size());
            QJsonArray::const_iterator keywordsIterator    = keywordsArray.constBegin();
            QJsonArray::const_iterator keywordsEndIterator = keywordsArray.constEnd();

            while (success && keywordsIterator != keywordsEndIterator) {
                QByteArray::FromBase64Result base64Result = QByteArray::fromBase64Encoding(
                    keywordsIterator->toString().toUtf8(),
                    QByteArray::Base64Option::AbortOnBase64DecodingErrors
                );

                if (base64Result) {
                    keywords.append(*base64Result);
                    ++keywordsIterator;
                } else {
                    success      = false;
                    statusString = QString(
                        "failed, keyword entries should be base64 encoded as per RFC4648, monitor ID %1"
                    ).arg(monitorId);
                }
            }
        } else {
            statusString = QString(
                "failed, keywords must be an array of RFC4648 base64 encoded values, monitor ID %1"
            ).arg(monitorId);
        }

        ++numberFields;
    }

    if (jsonData.contains("post_user_agent")) {
        QJsonValue userAgentValue = jsonData.value("post_user_agent");
        if (userAgentValue.isString()) {
            userAgent = userAgentValue.toString();
        } else {
            success      = false;
            statusString = QString("failed, post_user_agent must be a string, monitor ID %1").arg(monitorId);
        }

        ++numberFields;
    }

    if (jsonData.contains("post_content")) {
        QJsonValue postContentValue = jsonData.value("post_content");
        if (postContentValue.isString()) {
            QByteArray::FromBase64Result base64Result = QByteArray::fromBase64Encoding(
                postContentValue.toString().toUtf8(),
                QByteArray::Base64Option::AbortOnBase64DecodingErrors
            );

            if (base64Result) {
                postContent = *base64Result;
            } else {
                success      = false;
                statusString = QString(
                    "failed, post_user_agent should be base64 encoded as per RFC4648, monitor ID %1"
                ).arg(monitorId);
            }

        } else {
            success = false;
            statusString = QString(
                "failed, post_user_agent must be a string holding RFC4648 base64 encoded values, monitor ID %1"
            ).arg(monitorId);
        }

        ++numberFields;
    }

    if (numberFields != static_cast<unsigned>(jsonData.size())) {
        success      = false;
        statusString = QString("failed, unexpected entries, monitor ID %1").arg(monitorId);
    }

    if (success) {
        result = new Monitor(
            monitorId,
            uriString,
            method,
            contentCheckMode,
            keywords,
            contentType,
            userAgent,
            postContent
        );
    }

    return result;
}

/***********************************************************************************************************************
* InboundRestApi::CustomerRemove
*/

InboundRestApi::CustomerRemove::CustomerRemove(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::CustomerRemove::~CustomerRemove() {}


RestApiInV1::JsonResponse InboundRestApi::CustomerRemove::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("customer_id") && requestObject.size() == 1) {
            QJsonObject responseObject;

            double customerId = requestObject.value("customer_id").toDouble(-1);
            if (customerId >= 1 && customerId <= 0xFFFFFFFF) {
                bool success = currentServiceThreadTracker->removeCustomer(
                    static_cast<Customer::CustomerId>(customerId)
                );

                if (success) {
                    responseObject.insert("status", "OK");
                } else {
                    responseObject.insert("status", "failed, unknown customer ID");
                }
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* InboundRestApi::CustomerPause
*/

InboundRestApi::CustomerPause::CustomerPause(
        const QByteArray&     secret,
        ServiceThreadTracker* serviceThreadTracker
    ):RestApiInV1::InesonicRestHandler(
        secret
    ),currentServiceThreadTracker(
        serviceThreadTracker
    ) {}


InboundRestApi::CustomerPause::~CustomerPause() {}


RestApiInV1::JsonResponse InboundRestApi::CustomerPause::processAuthenticatedRequest(
        const QString&       /* path */,
        const QJsonDocument& request,
        unsigned             /* threadId */
    ) {
    RestApiInV1::JsonResponse response(StatusCode::BAD_REQUEST);

    if (request.isObject()) {
        QJsonObject requestObject = request.object();
        if (requestObject.contains("customer_id") && requestObject.contains("pause") && requestObject.size() == 2) {
            QJsonObject responseObject;

            bool   nowPaused  = requestObject.value("pause").toBool();
            double customerId = requestObject.value("customer_id").toDouble(-1);
            if (customerId >= 1 && customerId <= 0xFFFFFFFF) {
                currentServiceThreadTracker->setPaused(
                    static_cast<Customer::CustomerId>(customerId),
                    nowPaused
                );

                responseObject.insert("status", "OK");
            } else {
                responseObject.insert("status", "failed, invalid customer ID");
            }

            response = RestApiInV1::JsonResponse(responseObject);
        }
    }

    return response;
}

/***********************************************************************************************************************
* InboundRestApi
*/

const QString InboundRestApi::stateActivePath("/state/active");
const QString InboundRestApi::stateInactivePath("/state/inactive");
const QString InboundRestApi::regionChangePath("/region/change");
const QString InboundRestApi::loadingGetPath("/loading/get");
const QString InboundRestApi::customerAddPath("/customer/add");
const QString InboundRestApi::customerRemovePath("/customer/remove");
const QString InboundRestApi::customerPausePath("/customer/pause");

InboundRestApi::InboundRestApi(
        RestApiInV1::Server*  restApiServer,
        ServiceThreadTracker* serviceThreadTracker,
        const QByteArray&     secret,
        QObject*              parent
    ):QObject(
        parent
    ),stateActive(
        secret,
        serviceThreadTracker
    ),stateInactive(
        secret,
        serviceThreadTracker
    ),regionChange(
        secret,
        serviceThreadTracker
    ),loadingGet(
        secret,
        serviceThreadTracker
    ),customerAdd(
        secret,
        serviceThreadTracker
    ),customerRemove(
        secret,
        serviceThreadTracker
    ),customerPause(
        secret,
        serviceThreadTracker
    ) {
    restApiServer->registerHandler(&stateActive, RestApiInV1::Handler::Method::POST, stateActivePath);
    restApiServer->registerHandler(&stateInactive, RestApiInV1::Handler::Method::POST, stateInactivePath);
    restApiServer->registerHandler(&regionChange, RestApiInV1::Handler::Method::POST, regionChangePath);
    restApiServer->registerHandler(&loadingGet, RestApiInV1::Handler::Method::POST, loadingGetPath);
    restApiServer->registerHandler(&customerAdd, RestApiInV1::Handler::Method::POST, customerAddPath);
    restApiServer->registerHandler(&customerRemove, RestApiInV1::Handler::Method::POST, customerRemovePath);
    restApiServer->registerHandler(&customerPause, RestApiInV1::Handler::Method::POST, customerPausePath);
}


InboundRestApi::~InboundRestApi() {}


void InboundRestApi::setSecret(const QByteArray& newSecret) {
    stateActive.setSecret(newSecret);
    stateInactive.setSecret(newSecret);
    regionChange.setSecret(newSecret);
    loadingGet.setSecret(newSecret);
    customerAdd.setSecret(newSecret);
    customerRemove.setSecret(newSecret);
    customerPause.setSecret(newSecret);
}
