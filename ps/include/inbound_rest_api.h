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
* This header defines the \ref InboundRestApi class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef INBOUND_REST_API_H
#define INBOUND_REST_API_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_inesonic_rest_handler.h>

#include "loading_data.h"
#include "customer.h"
#include "host_scheme.h"
#include "monitor.h"

class ServiceThreadTracker;

/**
 * Class that support the polling server's inbound REST API.
 */
class InboundRestApi:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to change the state of this polling server to active.
         */
        static const QString stateActivePath;

        /**
         * Path used to change the state of this polling server to inactive.
         */
        static const QString stateInactivePath;

        /**
         * Path used to change the region settings related to this polling server.
         */
        static const QString regionChangePath;

        /**
         * Path used to obtain system loading information.
         */
        static const QString loadingGetPath;

        /**
         * Path used to add or change settings for a customer.
         */
        static const QString customerAddPath;

        /**
         * Path used to remove a customer.
         */
        static const QString customerRemovePath;

        /**
         * Path used to pause or resume polling for a customer.
         */
        static const QString customerPausePath;

        /**
         * Constructor
         *
         * \param[in] restApiServer        The REST API server instance.
         *
         * \param[in] serviceThreadTracker The service thread tracking API.
         *
         * \param[in[ secret               The inbound data secret.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        InboundRestApi(
            RestApiInV1::Server*  restApiServer,
            ServiceThreadTracker* serviceThreadTracker,
            const QByteArray&     secret,
            QObject*              parent = nullptr
        );

        ~InboundRestApi() override;

        /**
         * Method you can use to set the Inesonic authentication secret.
         *
         * \param[in] newSecret The new secret to be used.  The secret must be the length prescribed by the
         *                      constant \ref inesonicSecretLength.
         */
        void setSecret(const QByteArray& newSecret);

    private:
        /**
         * The state/active handler.
         */
        class StateActive:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                StateActive(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~StateActive() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The state/inactive handler.
         */
        class StateInactive:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                StateInactive(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~StateInactive() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The region/change handler.
         */
        class RegionChange:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                RegionChange(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~RegionChange() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The loading/get handler.
         */
        class LoadingGet:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                LoadingGet(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~LoadingGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * Method that generates a JSON object from loading data.
                 *
                 * \param[in] loadingData The loading data of interest.
                 *
                 * \return Returns a JSON object generated from the provided data.
                 */
                QJsonObject generateResponse(const QMap<unsigned, QList<LoadingData>>& loadingData);

                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The customer/add handler.
         */
        class CustomerAdd:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                CustomerAdd(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~CustomerAdd() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * Method that parses customer JSON data.
                 *
                 * \param[in,out] success      Flag that is set to false if an error is found.
                 *
                 * \param[in,out] statusString A string that is updated with status information if an error is found.
                 *
                 * \param[in]     customerId   The ID of the customer being created.
                 *
                 * \param[in]     jsonData     The JSON object describing the customer.
                 *
                 * \return Returns a pointer to a Customer instance containing the customer data.  A null pointer may
                 *         or may not be returned on error.  You are responsible for disposing of the returned data.
                 */
                Customer* generateCustomer(
                    bool&                success,
                    QString&             statusString,
                    Customer::CustomerId customerId,
                    const QJsonObject&   jsonData
                );

                /**
                 * Method that parses host/scheme JSON data.
                 *
                 * \param[in,out] success      Flag that is set to false if an error is found.
                 *
                 * \param[in,out] statusString A string that is updated with status information if an error is found.
                 *
                 * \param[in]     hostSchemeId The ID of the host/scheme being created.
                 *
                 * \param[in]     jsonData     The JSON object describing the customer.
                 *
                 * \return Returns a pointer to a \ref HostScheme instance containing the host/scheme data.  A null
                 *         pointer may or may not be returned on error.  You are responsible for disposing of the
                 *         returned data.
                 */
                HostScheme* generateHostScheme(
                    bool&                    success,
                    QString&                 statusString,
                    HostScheme::HostSchemeId hostSchemeId,
                    const QJsonObject&       jsonData
                );

                /**
                 * Method that parses monitor JSON data.
                 *
                 * \param[in,out] success      Flag that is set to false if an error is found.
                 *
                 * \param[in,out] statusString A string that is updated with status information if an error is found.
                 *
                 * \param[in]     monitorId    The ID of the monitor being created.
                 *
                 * \param[in]     jsonData     The JSON object describing the customer.
                 *
                 * \return Returns a pointer to a \ref Monitor instance containing the monitor data.  A null pointer
                 *         may or may not be returned on error.  You are responsible for disposing of the returned
                 *         data.
                 */
                Monitor* generateMonitor(
                    bool&              success,
                    QString&           statusString,
                    Monitor::MonitorId monitorId,
                    const QJsonObject& jsonData
                );

                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The customer/remove handler.
         */
        class CustomerRemove:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                CustomerRemove(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~CustomerRemove() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The customer/pause handler.
         */
        class CustomerPause:public RestApiInV1::InesonicRestHandler {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] secret               The secret to use for this handler.
                 *
                 * \param[in] serviceThreadTracker The service thread tracker.
                 */
                CustomerPause(const QByteArray& secret, ServiceThreadTracker* serviceThreadTracker);

                ~CustomerPause() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path     The request path.
                 *
                 * \param[in] request  The request data encoded as a JSON document.
                 *
                 * \param[in] threadId The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current service thread tracker.
                 */
                ServiceThreadTracker* currentServiceThreadTracker;
        };

        /**
         * The state/active handler.
         */
        StateActive stateActive;

        /**
         * The state/inactive handler.
         */
        StateInactive stateInactive;

        /**
         * The region/change handler.
         */
        RegionChange regionChange;

        /**
         * The loading/get handler.
         */
        LoadingGet loadingGet;

        /**
         * The customer/add handler.
         */
        CustomerAdd customerAdd;

        /**
         * The customer/remove handler.
         */
        CustomerRemove customerRemove;

        /**
         * The customer/pause handler.
         */
        CustomerPause customerPause;
};

#endif
