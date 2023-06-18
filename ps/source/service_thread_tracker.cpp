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
* This header implements the \ref ServiceThreadTracker class.
***********************************************************************************************************************/

#include <QThread>
#include <QSharedPointer>
#include <QHash>
#include <QMultiMap>
#include <QMutex>
#include <QMutexLocker>

#include <cstdint>
#include <iostream>

#include "customer.h"
#include "loading_data.h"
#include "data_aggregator.h"
#include "service_thread.h"
#include "http_service_thread.h"
#include "ping_service_thread.h"
#include "service_thread_tracker.h"

ServiceThreadTracker::ServiceThreadTracker(
        DataAggregator* dataAggregator,
        unsigned        maximumNumberThreads,
        QObject*        parent
    ):QObject(
        parent
    ),currentDataAggregator(
        dataAggregator
    ) {
    if (maximumNumberThreads == 0) {
        maximumNumberThreads = static_cast<unsigned>(QThread::idealThreadCount());
    }

    pingServiceThread = new PingServiceThread(this);

    unsigned numberHttpThreads = std::max(1U, maximumNumberThreads);
    for (unsigned i=0 ; i<numberHttpThreads ; ++i) {
        httpServiceThreads.append(new HttpServiceThread(dataAggregator, this));
    }

    currentStatus = Status::INACTIVE;
}


ServiceThreadTracker::~ServiceThreadTracker() {}


void ServiceThreadTracker::connectToPinger(const QString& socketName) {
    pingServiceThread->connectToPinger(socketName);
}


QMultiMap<int, LoadingData> ServiceThreadTracker::loadingData() const {
    QMultiMap<int, LoadingData> result;

    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    for (unsigned i=0 ; i<numberHttpThreads ; ++i) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(i);
        result.unite(serviceThread->loadingData());
    }

    return result;
}


float ServiceThreadTracker::monitorsPerSecond() const {
    float    result            = 0;
    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    for (unsigned i=0 ; i<numberHttpThreads ; ++i) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(i);
        result += serviceThread->hostSchemesPerSecond();
    }

    return result;
}


ServiceThreadTracker::Status ServiceThreadTracker::status() const {
    return currentStatus;
}


void ServiceThreadTracker::addCustomer(Customer* customer) {
    HttpServiceThread* bestHttpThread        = httpServiceThreads.at(0);
    float              bestMonitorsPerSecond = bestHttpThread->hostSchemesPerSecond();

    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    for (unsigned i=1 ; i<numberHttpThreads ; ++i) {
        HttpServiceThread* serviceThread     = httpServiceThreads.at(i);
        float              monitorsPerSecond = serviceThread->hostSchemesPerSecond();
        if (monitorsPerSecond < bestMonitorsPerSecond) {
            bestHttpThread        = serviceThread;
            bestMonitorsPerSecond = monitorsPerSecond;
        }
    }

    bestHttpThread->addCustomer(customer);

    if (customer->supportsPingTesting()) {
        Customer::CustomerId customerId  = customer->customerId();
        QList<HostScheme*>   hostSchemes = customer->hostSchemes();
        for (  QList<HostScheme*>::const_iterator it = hostSchemes.constBegin(), end = hostSchemes.constEnd()
             ; it != end
             ; ++it
            ) {
            HostScheme*     hostScheme = *it;
            pingServiceThread->addHost(
                customerId,
                hostScheme->url(),
                QPointer<HostScheme>(hostScheme),
                bestHttpThread
            );
        }
    }

    std::cout << "Added customer " << customer->customerId() << ", "
              << "ping: " << (customer->supportsPingTesting() ? "true" : "false" ) << ", "
              << "ssl: " << (customer->supportsSslExpirationChecking() ? "true" : "false" ) << ", "
              << "latency: " << (customer->supportsLatencyMeasurements() ? "true" : "false") << ", "
              << "mult-region: " << (customer->supportsMultiRegionTesting() ? "true" : "false") << ", "
              << "polling-interval: " << customer->pollingInterval() << " sec, "
              << "paused: " << (customer->paused() ? "true, " : "false, ")
              << "hosts: " << customer->numberHostSchemes() << ", "
              << "monitors: " << customer->numberMonitors() << std::endl;
}


bool ServiceThreadTracker::removeCustomer(Customer::CustomerId customerId) {
    bool     success           = false;
    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    unsigned index             = 0;

    while (!success && index < numberHttpThreads) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(index);
        success = serviceThread->removeCustomer(customerId);
        ++index;
    }

    pingServiceThread->removeCustomer(customerId);

    std::cout << "Removed customer " << customerId << std::endl;

    return success;
}


Customer* ServiceThreadTracker::getCustomer(Customer::CustomerId customerId) const {
    Customer* customer          = nullptr;
    unsigned  numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    unsigned  index             = 0;

    while (customer == nullptr && index < numberHttpThreads) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(index);
        customer = serviceThread->getCustomer(customerId);
        ++index;
    }

    return customer;
}


HostScheme* ServiceThreadTracker::getHostScheme(HostScheme::HostSchemeId hostSchemeId) const {
    HostScheme* hostScheme        = nullptr;
    unsigned    numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    unsigned    index             = 0;

    while (hostScheme == nullptr && index < numberHttpThreads) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(index);
        hostScheme = serviceThread->getHostScheme(hostSchemeId);
        ++index;
    }

    return hostScheme;
}


Monitor* ServiceThreadTracker::getMonitor(Monitor::MonitorId monitorId) const {
    Monitor* monitor           = nullptr;
    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    unsigned index             = 0;

    while (monitor == nullptr && index < numberHttpThreads) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(index);
        monitor = serviceThread->getMonitor(monitorId);
        ++index;
    }

    return monitor;
}


bool ServiceThreadTracker::paused(Customer::CustomerId customerId) const {
    bool      result   = false;
    Customer* customer = getCustomer(customerId);
    if (customer != nullptr) {
        result = customer->paused();
    }

    return result;
}


void ServiceThreadTracker::setPaused(Customer::CustomerId customerId, bool nowPaused) {
    Customer* customer = getCustomer(customerId);
    if (customer != nullptr) {
        customer->setPaused(nowPaused);
    }
}


QString ServiceThreadTracker::toString(Status status) {
    QString result;

    switch (status) {
        case Status::ALL_UNKNOWN: { result = QString("ALL_UNKNOWN");     break; }
        case Status::INACTIVE:    { result = QString("INACTIVE");        break; }
        case Status::ACTIVE:      { result = QString("ACTIVE");          break; }
        case Status::DEFUNCT:     { result = QString("DEFUNCT");         break; }
        default: {
            Q_ASSERT(false);
            break;
        }
    }

    return result;
}

void ServiceThreadTracker::updateRegionData(unsigned regionIndex, unsigned numberRegions) {
    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    for (unsigned i=0 ; i<numberHttpThreads ; ++i) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(i);
        serviceThread->updateRegionData(regionIndex, numberRegions);
    }

    pingServiceThread->goActive();

    std::cout << "Changing region to " << regionIndex << " / " << numberRegions << std::endl;

    if (currentStatus != Status::ACTIVE) {
        std::cout << "-------------------------------------------------------------------------------" << std::endl
                  << toString(currentStatus).toLocal8Bit().data() << " -> ACTIVE (Region Change)" << std::endl
                  << "-------------------------------------------------------------------------------" << std::endl;
    }

    currentStatus = Status::ACTIVE;
    currentDataAggregator->sendReport();
}


void ServiceThreadTracker::goActive(bool nowActive) {
    unsigned numberHttpThreads = static_cast<unsigned>(httpServiceThreads.size());
    for (unsigned i=0 ; i<numberHttpThreads ; ++i) {
        HttpServiceThread* serviceThread = httpServiceThreads.at(i);
        if (nowActive) {
            serviceThread->goActive();
        } else {
            serviceThread->goInactive();
        }
    }

    pingServiceThread->goInactive();

    Status newStatus = nowActive ? Status::ACTIVE : Status::INACTIVE;
    if (newStatus != currentStatus)
    std::cout << "-------------------------------------------------------------------------------" << std::endl
              << toString(currentStatus).toLocal8Bit().data() << " -> "
                 << toString(newStatus).toLocal8Bit().data() << std::endl
              << "-------------------------------------------------------------------------------" << std::endl;

    currentStatus = newStatus;

    currentDataAggregator->sendReport();
}


void ServiceThreadTracker::goInactive(bool nowInactive) {
    goActive(!nowInactive);
}
