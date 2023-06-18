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
* This header defines the \ref PingServiceThread class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef PING_SERVICE_THREAD_H
#define PING_SERVICE_THREAD_H

#include <QPointer>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QHostAddress>

#include <cstdint>

#include "customer.h"
#include "host_scheme.h"
#include "service_thread.h"

class QLocalSocket;
class HttpServiceThread;
class PingServiceThreadPrivate;

/**
 * Class that manages ping services.
 */
class PingServiceThread:public ServiceThread {
    Q_OBJECT

    friend class Customer;

    public:
        /**
         * Constructor.
         *
         * \param[in] parent Pointer to the parent object.
         */
        PingServiceThread(QObject* parent = nullptr);

        ~PingServiceThread() override;

        /**
         * Method you can use to connect to the pinger.
         *
         * \param[in] socketName The name of the local Linux socket.
         */
        void connect(const QString& socketName);

        /**
         * Method you can use to determine the number of hosts tied to this ping server.
         *
         * \return Returns the number of hosts tied to this ping server.
         */
        unsigned long numberHosts() const {
            return 0;
        }

        /**
         * Method you can use to add a host to this service thread tied to a specific customer.
         *
         * \param[in] customerId        The customer ID of the customer tied to this host.
         *
         * \param[in] hostUrl           The host address of the host to be pinged.
         *
         * \param[in] hostScheme        The host/scheme of the host being pinged.
         *
         * \param[in] httpServiceThread The service thread that is managing this host/scheme.
         */
        void addHost(
            Customer::CustomerId customerId,
            const QUrl&          hostUrl,
            QPointer<HostScheme> hostScheme,
            HttpServiceThread*   httpServiceThread
        );

        /**
         * Method you can use to remove a customer from this service thread.
         *
         * \param[in] customerId The zero based ID of the customer to be removed.
         */
        void removeCustomer(Customer::CustomerId customerId);

    signals:
        /**
         * Signal that is used to perform connections across threads.
         *
         * \param[in] socketName The socket name to connect to.
         */
        void connectToPinger(const QString& socketName);

    public slots:
        /**
         * Slot you can trigger to command this thread to go inactive.
         */
        void goInactive() override;

        /**
         * Slot you can trigger to command this thread to go active.
         */
        void goActive() override;

    protected:
        /**
         * Method you can use to run this thread.  The method simply starts the thread's exec loop.
         */
        void run() override;

    private:
        /**
         * The underlying private instance.  Runs within this thread.
         */
        PingServiceThreadPrivate* impl;
};

#endif
