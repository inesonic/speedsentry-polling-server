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
* This header implements the \ref HttpServiceThread class.
***********************************************************************************************************************/

#include <QThread>
#include <QNetworkAccessManager>
#include <QSharedPointer>
#include <QHash>
#include <QMultiMap>
#include <QMutex>
#include <QMutexLocker>

#include <cstdint>

#include "customer.h"
#include "data_aggregator.h"
#include "loading_data.h"
#include "host_scheme_timer.h"
#include "service_thread.h"
#include "http_service_thread.h"

HttpServiceThread::HttpServiceThread(DataAggregator* dataAggregator, QObject* parent):ServiceThread(parent) {
    currentDataAggregator = dataAggregator;

    networkAccessManager = new QNetworkAccessManager;
    networkAccessManager->setRedirectPolicy(QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);
    networkAccessManager->setStrictTransportSecurityEnabled(true);
    networkAccessManager->moveToThread(this);

    currentThreadObject = new QObject;
    currentThreadObject->moveToThread(this);

    currentRegionIndex          = 0;
    currentNumberRegions        = 0;
    currentActive               = false;
    currentHostSchemesPerSecond = 0;

    start();
}


HttpServiceThread::~HttpServiceThread() {
    quit();
    wait();

    delete currentThreadObject;
    delete networkAccessManager;

    for (  CustomersByCustomerId::const_iterator it  = customersByCustomerId.constBegin(),
                                                 end = customersByCustomerId.constEnd()
         ; it != end
         ; ++it
        ) {
        delete it.value();
    }
}


float HttpServiceThread::hostSchemesPerSecond() const {
    return currentHostSchemesPerSecond;
}


QMultiMap<int, LoadingData> HttpServiceThread::loadingData() const {
    QMultiMap<int, LoadingData> result;
    for (  QMap<int, HostSchemeTimer*>::const_iterator it  = hostSchemeTimers.constBegin(),
                                                       end = hostSchemeTimers.constEnd()
         ; it != end
         ; ++it
        ) {
        HostSchemeTimer* hostSchemeTimer = it.value();
        result.insert(it.key(), hostSchemeTimer->loadingData());
    }

    return result;
}


void HttpServiceThread::addCustomer(Customer* customer) {
    customer->currentServiceThread = this;
    customerAdded(customer);
}


bool HttpServiceThread::removeCustomer(Customer::CustomerId customerId) {
    bool success;

    customerMutex.lock();
    CustomersByCustomerId::iterator it = customersByCustomerId.find(customerId);
    if (it != customersByCustomerId.end()) {
        Customer* customer = it.value();

        customersByCustomerId.erase(it);
        customerMutex.unlock();

        customer->reportExistingHostSchemesAndMonitors(this, false);

        delete customer;
        success = true;
    } else {
        customerMutex.unlock();
        success = false;
    }

    return success;
}


Customer* HttpServiceThread::getCustomer(Customer::CustomerId customerId) const {
    QMutexLocker locker(&customerMutex);
    return customersByCustomerId.value(customerId);
}


HostScheme* HttpServiceThread::getHostScheme(HostScheme::HostSchemeId hostSchemeId) const {
    QMutexLocker locker(&hostSchemeMutex);

    QMap<int, HostSchemeTimer*>::const_iterator it     = hostSchemeTimers.constBegin();
    QMap<int, HostSchemeTimer*>::const_iterator end    = hostSchemeTimers.constEnd();
    HostScheme*                                 result = nullptr;
    while (result == nullptr && it != end) {
        const HostSchemeTimer* hostSchemeTimer = it.value();
        result = hostSchemeTimer->getHostScheme(hostSchemeId);
        ++it;
    }

    return result;
}


Monitor* HttpServiceThread::getMonitor(Monitor::MonitorId monitorId) const {
    QMutexLocker locker(&monitorMutex);
    return monitorsByMonitorId.value(monitorId);
}


void HttpServiceThread::checkNow(QPointer<HostScheme> hostScheme) {
    QMutexLocker locker(&hostSchemeMutex);
    if (!hostScheme.isNull()) {
        hostScheme->startCheckFromDifferentThread();
    }
}


void HttpServiceThread::updateRegionData(unsigned regionIndex, unsigned numberRegions) {
    currentRegionIndex   = regionIndex;
    currentNumberRegions = numberRegions;
    currentActive        = true;

    for (  QMap<int, HostSchemeTimer*>::const_iterator it  = hostSchemeTimers.constBegin(),
                                                       end = hostSchemeTimers.constEnd()
         ; it != end
         ; ++it
        ) {
        HostSchemeTimer* hostSchemeTimer = it.value();
        hostSchemeTimer->updateRegionData(regionIndex, numberRegions);
    }
}


void HttpServiceThread::goInactive() {
    currentActive = false;

    for (  QMap<int, HostSchemeTimer*>::const_iterator it  = hostSchemeTimers.constBegin(),
                                                       end = hostSchemeTimers.constEnd()
         ; it != end
         ; ++it
        ) {
        HostSchemeTimer* hostSchemeTimer = it.value();
        hostSchemeTimer->goInactive();
    }
}


void HttpServiceThread::goActive() {
    currentActive = true;
    for (  QMap<int, HostSchemeTimer*>::const_iterator it  = hostSchemeTimers.constBegin(),
                                                       end = hostSchemeTimers.constEnd()
         ; it != end
         ; ++it
        ) {
        HostSchemeTimer* hostSchemeTimer = it.value();
        hostSchemeTimer->goActive();

    }
}


void HttpServiceThread::run() {
    exec();
}


void HttpServiceThread::monitorAdded(Monitor* monitor) {
    QMutexLocker locker(&monitorMutex);
    monitorsByMonitorId.insert(monitor->monitorId(), monitor);
}


void HttpServiceThread::monitorAboutToBeRemoved(Monitor* monitor) {
    QMutexLocker locker(&monitorMutex);
    monitorsByMonitorId.remove(monitor->monitorId());
}


void HttpServiceThread::hostSchemeAdded(HostScheme* hostScheme) {
    hostSchemeMutex.lock();

    Customer*                             customer              = hostScheme->customer();
    unsigned                              pollingInterval       = customer->pollingInterval();
    bool                                  multiRegion           = customer->supportsMultiRegionTesting();
    int                                   signedPollingInterval = multiRegion ? pollingInterval : -pollingInterval;
    QMap<int, HostSchemeTimer*>::iterator it                    = hostSchemeTimers.find(signedPollingInterval);
    HostSchemeTimer*                      hostSchemeTimer       = nullptr;

    if (it != hostSchemeTimers.end()) {
        hostSchemeTimer = it.value();
    } else {
        hostSchemeTimer = new HostSchemeTimer(
            multiRegion,
            pollingInterval,
            currentRegionIndex,
            currentNumberRegions,
            currentActive
        );

        hostSchemeTimer->moveToThread(this);
        hostSchemeTimers.insert(signedPollingInterval, hostSchemeTimer);
    }

    hostScheme->setNetworkAccessManager(networkAccessManager);
    hostSchemeTimer->addHostScheme(hostScheme);

    hostSchemeMutex.unlock();
    updateServiceMetrics();
}


void HttpServiceThread::hostSchemeAboutToBeRemoved(HostScheme* hostScheme) {
    hostSchemeMutex.lock();

    Customer*                             customer              = hostScheme->customer();
    unsigned                              pollingInterval       = customer->pollingInterval();
    bool                                  multiRegion           = customer->supportsMultiRegionTesting();
    int                                   signedPollingInterval = multiRegion ? pollingInterval : -pollingInterval;
    QMap<int, HostSchemeTimer*>::iterator it                    = hostSchemeTimers.find(signedPollingInterval);

    if (it != hostSchemeTimers.end()) {
        HostSchemeTimer* hostSchemeTimer = it.value();
        hostSchemeTimer->removeHostScheme(hostScheme->hostSchemeId());
    }

    hostSchemeMutex.unlock();

    updateServiceMetrics();
}


void HttpServiceThread::customerAdded(Customer* customer) {
    QMutexLocker locker(&customerMutex);
    customersByCustomerId.insert(customer->customerId(), customer);
    customer->moveToThread(this);

    customer->reportExistingHostSchemesAndMonitors(this, true);
}


void HttpServiceThread::customerAboutToBeRemoved(Customer* customer) {
    QMutexLocker locker(&customerMutex);
    customersByCustomerId.remove(customer->customerId());

    customer->reportExistingHostSchemesAndMonitors(this, false);
}


void HttpServiceThread::updateServiceMetrics() {
    QMutexLocker locker(&monitorMutex);

    float newHostSchemesPerSecond = 0;
    for (  QMap<int, HostSchemeTimer*>::const_iterator it  = hostSchemeTimers.constBegin(),
                                                       end = hostSchemeTimers.constEnd()
         ; it != end
         ; ++it
        ) {
        newHostSchemesPerSecond += it.value()->monitorsPerSecond();
    }

    currentHostSchemesPerSecond = newHostSchemesPerSecond;
}
