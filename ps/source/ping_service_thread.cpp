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
* This header implements the \ref PingServiceThread class.
***********************************************************************************************************************/

#include <QThread>
#include <QHash>
#include <QLocalSocket>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QVector>
#include <QPair>
#include <QStringList>

#include <cstdint>
#include <netdb.h>

#include "log.h"
#include "customer.h"
#include "data_aggregator.h"
#include "service_thread.h"
#include "http_service_thread.h"
#include "ping_service_thread_private.h"
#include "ping_service_thread.h"

PingServiceThread::PingServiceThread(QObject* parent):ServiceThread(parent) {
    impl = new PingServiceThreadPrivate;
    QObject::connect(this, &PingServiceThread::connectToPinger, impl, &PingServiceThreadPrivate::connectToPinger);
}


PingServiceThread::~PingServiceThread() {
    quit();
    wait();
}


void PingServiceThread::connect(const QString& socketName) {
    emit connectToPinger(socketName);
}


void PingServiceThread::addHost(
        Customer::CustomerId customerId,
        const QUrl&          hostUrl,
        QPointer<HostScheme> hostScheme,
        HttpServiceThread*   httpServiceThread
    ) {
    impl->addHost(customerId, hostUrl, hostScheme, httpServiceThread);
}


void PingServiceThread::removeCustomer(Customer::CustomerId customerId) {
    impl->removeCustomer(customerId);
}


void PingServiceThread::goInactive() {
    impl->goInactive();
}


void PingServiceThread::goActive() {
    impl->goActive();
}


void PingServiceThread::run() {
    impl->moveToThread(this);
    exec();
}
