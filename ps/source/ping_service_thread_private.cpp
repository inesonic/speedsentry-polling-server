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
* This header implements the \ref PingServiceThreadPrivate class.
***********************************************************************************************************************/

#include <QObject>
#include <QSet>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QLocalSocket>
#include <QTimer>
#include <QHash>
#include <QList>

#include <cstdint>
#include <netdb.h>

#include "log.h"
#include "customer.h"
#include "data_aggregator.h"
#include "service_thread.h"
#include "http_service_thread.h"
#include "ping_service_thread_private.h"


PingServiceThreadPrivate::PingServiceThreadPrivate(QObject* parent):QObject(parent) {
    retryTimer = new QTimer(this);
    retryTimer->setSingleShot(true);
    connect(retryTimer, &QTimer::timeout, this, &PingServiceThreadPrivate::issueNextCommand);

    socket = new QLocalSocket(this);
    connect(socket, &QLocalSocket::readyRead, this, &PingServiceThreadPrivate::readyRead);
    connect(socket, &QLocalSocket::readChannelFinished, this, &PingServiceThreadPrivate::readChannelFinished);

    connect(this, &PingServiceThreadPrivate::startNextCommand, this, &PingServiceThreadPrivate::issueNextCommand);
}


PingServiceThreadPrivate::~PingServiceThreadPrivate() {}


unsigned long PingServiceThreadPrivate::numberHosts() const {
    QMutexLocker locker(&hostSchemeMutex);
    return static_cast<unsigned long>(hostDataByHostSchemeId.size());
}


void PingServiceThreadPrivate::addHost(
        Customer::CustomerId customerId,
        const QUrl&          hostUrl,
        QPointer<HostScheme> hostScheme,
        HttpServiceThread*   httpServiceThread
    ) {
    HostData hostData(hostUrl, hostScheme, httpServiceThread);

    QMutexLocker locker(&hostSchemeMutex);

    HostDataByHostSchemeId::iterator hit = hostDataByHostSchemeId.find(hostScheme->hostSchemeId());
    if (hit == hostDataByHostSchemeId.end()) {
        hostDataByHostSchemeId.insert(hostScheme->hostSchemeId(), hostData);
        HostSchemeIdsByCustomerId::iterator cit = hostSchemeIdsByCustomerId.find(customerId);
        if (cit != hostSchemeIdsByCustomerId.end()) {
            cit.value().insert(hostScheme->hostSchemeId());
        } else {
            hostSchemeIdsByCustomerId.insert(
                customerId,
                QSet<HostScheme::HostSchemeId>() << hostScheme->hostSchemeId()
            );
        }

        issueCommand(CommandEntry(CommandEntry::Command::ADD, hostScheme->hostSchemeId(), hostScheme->url().host()));

    }
}


void PingServiceThreadPrivate::removeCustomer(Customer::CustomerId customerId) {
    QMutexLocker locker(&hostSchemeMutex);

    HostSchemeIdsByCustomerId::iterator cit = hostSchemeIdsByCustomerId.find(customerId);
    if (cit != hostSchemeIdsByCustomerId.end()) {
        const QSet<HostScheme::HostSchemeId>& hostSchemeIds = cit.value();
        for (  QSet<HostScheme::HostSchemeId>::const_iterator hit  = hostSchemeIds.constBegin(),
                                                              hend = hostSchemeIds.constEnd()
             ; hit != hend
             ; ++hit
            ) {
            issueCommand(CommandEntry(CommandEntry::Command::REMOVE, *hit));
            hostDataByHostSchemeId.remove(*hit);
        }

        hostSchemeIdsByCustomerId.erase(cit);
    }
}


void PingServiceThreadPrivate::connectToPinger(const QString& socketName) {
    socket->connectToServer(socketName, QLocalSocket::OpenModeFlag::ReadWrite);
    if (socket->state() != QLocalSocket::LocalSocketState::UnconnectedState) {
        currentSocketName = socketName;
        logWrite("Connecting to pinger");
    } else {
        logWrite("Failed to connect to pinger.");
    }
}


void PingServiceThreadPrivate::goInactive() {
    QMutexLocker locker(&hostSchemeMutex);

    for (  HostDataByHostSchemeId::const_iterator it  = hostDataByHostSchemeId.constBegin(),
                                                  end = hostDataByHostSchemeId.constEnd()
         ; it != end
         ; ++it
        ) {
        issueCommand(CommandEntry(CommandEntry::Command::REMOVE, it.key()));
    }
}


void PingServiceThreadPrivate::goActive() {
    QMutexLocker locker(&hostSchemeMutex);

    for (  HostDataByHostSchemeId::const_iterator it  = hostDataByHostSchemeId.constBegin(),
                                                  end = hostDataByHostSchemeId.constEnd()
         ; it != end
         ; ++it
        ) {
        issueCommand(CommandEntry(CommandEntry::Command::ADD, it.key(), it.value().hostScheme()->url().host()));
    }
}


void PingServiceThreadPrivate::readyRead() {
    if (socket->canReadLine()) {
        char line[maximumLineLength + 1];
        qint64 bytesRead = socket->readLine(line, maximumLineLength);
        if (bytesRead >= 0) {
            QString received = QString::fromUtf8(line).trimmed();
            if (received == QString("OK")) {
                commandMutex.lock();
                pendingCommands.removeFirst();
                commandMutex.unlock();

                issueNextCommand();
            } else if (received.startsWith("NOPING ")) {
                // Handle noping message
            } else if (received.startsWith("ERROR ")) {
                logWrite(
                    QString("Pinger reported error, command \"%1\", response \"%2\", ignoring.")
                    .arg(commandString(pendingCommands.first()))
                    .arg(received),
                    true
                );

                commandMutex.lock();
                pendingCommands.removeFirst();
                commandMutex.unlock();

                issueNextCommand();
            } else if (received.startsWith("failed")) {
                logWrite(
                    QString("Pinger reported error, command \"%1\", response \"%2\", will retry.")
                    .arg(commandString(pendingCommands.first()))
                    .arg(received),
                    true
                );

                retryTimer->start(pingerRetryDelay);
            }
        }
    }
}


void PingServiceThreadPrivate::readChannelFinished() {
    logWrite(QString("Pinger disconnected unexpectedly."), true);
    retryTimer->start(pingerRetryDelay);
}


void PingServiceThreadPrivate::issueNextCommand() {
    if (socket->state() == QLocalSocket::LocalSocketState::ConnectedState) {
        QMutexLocker locker(&commandMutex);

        if (!pendingCommands.isEmpty()) {
            const CommandEntry& nextCommand = pendingCommands.first();

            QString cmd = commandString(nextCommand);
            logWrite(QString("Issuing pinger command \"%1\"").arg(cmd));

            cmd += "\n";
            socket->write(cmd.toUtf8());
        }
    } else {
        if (socket->state() == QLocalSocket::LocalSocketState::UnconnectedState) {
            socket->connectToServer(currentSocketName, QLocalSocket::OpenModeFlag::ReadWrite);
        }

        retryTimer->start(pingerRetryDelay);
    }
}


void PingServiceThreadPrivate::issueCommand(const CommandEntry& commandEntry) {
    QMutexLocker locker(&commandMutex);

    if (pendingCommands.isEmpty()) {
        pendingCommands.append(commandEntry);
        emit startNextCommand();
    } else {
        pendingCommands.append(commandEntry);
    }
}


QString PingServiceThreadPrivate::commandString(const CommandEntry& commandEntry) {
    QString result;

    switch (commandEntry.command()) {
        case CommandEntry::Command::ADD: {
            result = QString("A %1 %2").arg(commandEntry.hostId()).arg(commandEntry.serverName());
            break;
        }

        case CommandEntry::Command::REMOVE: {
            result = QString("R %1").arg(commandEntry.hostId());
            break;
        }

        case CommandEntry::Command::DEFUNCT: {
            result = QString("D %1").arg(commandEntry.hostId());
            break;
        }

        default: {
            logWrite(
                QString("Unexpected pinger command %1.").arg(static_cast<unsigned>(commandEntry.command())),
                true
            );

            break;
        }
    }

    return result;
}
