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
* This header defines the \ref PingServiceThreadPrivate class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef PING_SERVICE_THREAD_PRIVATE_H
#define PING_SERVICE_THREAD_PRIVATE_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QSet>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QMutex>

#include "customer.h"
#include "host_scheme.h"
#include "service_thread.h"

class QLocalSocket;
class QTimer;

class HttpServiceThread;

/**
 * Class that communicates with the pinger server.
 */
class PingServiceThreadPrivate:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor.
         *
         * \param[in] parent Pointer to the parent object.
         */
        PingServiceThreadPrivate(QObject* parent = nullptr);

        ~PingServiceThreadPrivate() override;

        /**
         * Method you can use to determine the number of hosts tied to this ping server.
         *
         * \return Returns the number of hosts tied to this ping server.
         */
        unsigned long numberHosts() const;

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
         * Signal that causes the next command to be started.
         */
        void startNextCommand();

    public slots:
        /**
         * Slot you can use to connect to the pinger.
         *
         * \param[in] socketName The name of the socket to connect to.
         */
        void connectToPinger(const QString& socketName);

        /**
         * Slot you can trigger to command this thread to go inactive.
         */
        void goInactive();

        /**
         * Slot you can trigger to command this thread to go active.
         */
        void goActive();

    private slots:
        /**
         * Slot that is triggered when the local socket has data available.
         */
        void readyRead();

        /**
         * Slot that is triggered when the local socket disconnects.
         */
        void readChannelFinished();

        /**
         * Slot that issues the next command to the pinger.
         */
        void issueNextCommand();

    private:
        /**
         * The maximum allowed received message length, in bytes.
         */
        static constexpr unsigned maximumLineLength = 512;

        /**
         * Pinger retry delay.
         */
        static constexpr unsigned pingerRetryDelay = 10000;

        /**
         * Class used to track the status for this host.
         */
        class HostData {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] hostUrl           The URL for this host.
                 *
                 * \param[in] hostScheme        The host/scheme to be checked if pings fails for this host.
                 *
                 * \param[in] httpServiceThread The service thread tracking this host/scheme.
                 */
                inline HostData(
                        const QUrl&          hostUrl,
                        QPointer<HostScheme> hostScheme,
                        HttpServiceThread*   httpServiceThread
                    ):currentUrl(
                        hostUrl
                    ),currentHostScheme(
                        hostScheme
                    ),currentHttpServiceThread(
                        httpServiceThread
                    ) {}

                /**
                 * Copy constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline HostData(
                        const HostData& other
                    ):currentUrl(
                        other.currentUrl
                    ),currentHostScheme(
                        other.currentHostScheme
                    ),currentHttpServiceThread(
                        other.currentHttpServiceThread
                    ) {}

                ~HostData() = default;

                /**
                 * Method you can use to get this host URL.
                 *
                 * \return Returns this host's address.
                 */
                inline const QUrl& url() const {
                    return currentUrl;
                }

                /**
                 * Method you can use to get the host/scheme instance for this host.
                 *
                 * \return Returns the host/scheme for this host.
                 */
                inline QPointer<HostScheme> hostScheme() const {
                    return currentHostScheme;
                }

                /**
                 * Method you can use to get the HTTP service thread that is tracking the supplied host/scheme.
                 *
                 * \return Returns a pointer to the HTTP service thread tracking this host/scheme.
                 */
                inline HttpServiceThread* httpServiceThread() const {
                    return currentHttpServiceThread;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                HostData& operator=(const HostData& other) {
                    currentUrl               = other.currentUrl;
                    currentHostScheme        = other.currentHostScheme;
                    currentHttpServiceThread = other.currentHttpServiceThread;

                    return *this;
                }

            private:
                /**
                 * The URL for this host.
                 */
                QUrl currentUrl;

                /**
                 * The host/scheme to check on failure.
                 */
                QPointer<HostScheme> currentHostScheme;

                /**
                 * The HTTP service thread tracking this host/scheme.
                 */
                HttpServiceThread* currentHttpServiceThread;
        };

        /**
         * Class that represents an individual pinger command.
         */
        class CommandEntry {
            public:
                /**
                 * Enumeration of supported commands.
                 */
                enum class Command {
                    /**
                     * Indicates we wish to add this host.
                     */
                    ADD,

                    /**
                     * Indicates we wish to remove this host.
                     */
                    REMOVE,

                    /**
                     * Indicates we wish to mark this host as defunct.
                     */
                    DEFUNCT
                };

                /**
                 * Constructor
                 *
                 * \param[in] command    The command to be executed.
                 *
                 * \param[in] hostId     The ID of the host, used for tracking.
                 *
                 * \param[in] serverName The server name of the server.  Only used with the "ADD" command.
                 */
                inline CommandEntry(
                        Command                  command,
                        HostScheme::HostSchemeId hostId,
                        const QString&           serverName = QString()
                    ):currentCommand(
                        command
                    ),currentHostId(
                        hostId
                    ),currentServerName(
                        serverName
                    ) {}

                /**
                 * Copy constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline CommandEntry(
                        const CommandEntry& other
                    ):currentCommand(
                        other.currentCommand
                    ),currentHostId(
                        other.currentHostId
                    ),currentServerName(
                        other.currentServerName
                    ) {}

                /**
                 * Move constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline CommandEntry(
                        CommandEntry&& other
                    ):currentCommand(
                        other.currentCommand
                    ),currentHostId(
                        other.currentHostId
                    ),currentServerName(
                        other.currentServerName
                    ) {}

                ~CommandEntry() {}

                /**
                 * Method you can use to obtain the current command.
                 *
                 * \return returns the command value.
                 */
                inline Command command() const {
                    return currentCommand;
                }

                /**
                 * Method you can use to obtain the host ID.
                 *
                 * \return Returns the host ID.
                 */
                inline HostScheme::HostSchemeId hostId() const {
                    return currentHostId;
                }

                /**
                 * Method you can use to obtain the server name.
                 *
                 * \return Returns the current server name.
                 */
                inline const QString& serverName() const {
                    return currentServerName;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                CommandEntry& operator=(const CommandEntry& other) {
                    currentCommand    = other.currentCommand;
                    currentHostId     = other.currentHostId;
                    currentServerName = other.currentServerName;

                    return *this;
                }

                /**
                 * Assignment operator (move semantics)
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                CommandEntry& operator=(CommandEntry&& other) {
                    currentCommand    = other.currentCommand;
                    currentHostId     = other.currentHostId;
                    currentServerName = other.currentServerName;

                    return *this;
                }

            private:
                /**
                 * The pending command.
                 */
                Command currentCommand;

                /**
                 * The pending host ID.
                 */
                HostScheme::HostSchemeId currentHostId;

                /**
                 * The pending server name.
                 */
                QString currentServerName;
        };

        /**
         * Type used to track pending commands.
         */
        typedef QList<CommandEntry> PendingCommands;

        /**
         * Type used to track hosts managed by each customer.
         */
        typedef QHash<Customer::CustomerId, QSet<HostScheme::HostSchemeId>> HostSchemeIdsByCustomerId;

        /**
         * Type used to track host data by host/scheme ID.
         */
        typedef QHash<HostScheme::HostSchemeId, HostData> HostDataByHostSchemeId;

        /**
         * Method that is used to issue a new command.
         *
         * \param[in] commandEntry The new command to be issued.
         */
        void issueCommand(const CommandEntry& commandEntry);

        /**
         * Method that converts a command entry to a pinger command string.
         *
         * \param[in] commandEntry The entry to be converted.
         *
         * \return Returns the entry converted to a string.
         */
        static QString commandString(const CommandEntry& commandEntry);

        /**
         * The socket name of the socket to connect to.
         */
        QString currentSocketName;

        /**
         * Local socket used to communicate with the pinger.
         */
        QLocalSocket* socket;

        /**
         * Timer used to retry the pinger.
         */
        QTimer* retryTimer;

        /**
         * Mutex used to keep our commands in a thread-safe manner.
         */
        mutable QMutex commandMutex;

        /**
         * Value used to track pending commands.
         */
        PendingCommands pendingCommands;

        /**
         * Mutex used to keep our host/scheme structures thread safe.
         */
        mutable QMutex hostSchemeMutex;

        /**
         * Type used to track host schemes by customer ID.
         */
        HostSchemeIdsByCustomerId hostSchemeIdsByCustomerId;

        /**
         * Type used to track host data instances by host/scheme ID.
         */
        HostDataByHostSchemeId hostDataByHostSchemeId;

        /**
         * Flag indicating if the ping threads should be inactive.
         */
        bool activeMode;
};

#endif
