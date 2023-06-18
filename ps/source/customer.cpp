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
* This header implements the \ref Customer class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

#include <cstdint>

#include "monitor.h"
#include "host_scheme.h"
#include "http_service_thread.h"
#include "customer.h"

Customer::Customer(
        Customer::CustomerId customerId,
        bool                 supportsPingTesting,
        bool                 supportsSslExpirationChecking,
        bool                 supportsLatencyMeasurements,
        bool                 supportsMultiRegionTesting,
        unsigned             pollingInterval,
        HttpServiceThread*   serviceThread
    ):currentServiceThread(
        serviceThread
    ),currentCustomerId(
        customerId
    ),currentSupportsPingTesting(
        supportsPingTesting
    ),currentSupportsSslExpirationChecking(
        supportsSslExpirationChecking
    ),currentSupportsLatencyMeasurements(
        supportsLatencyMeasurements
    ),currentSupportsMultiRegionTesting(
        supportsMultiRegionTesting
    ),currentPollingInterval(
        pollingInterval
    ) {
    currentlyPaused = false;

    if (serviceThread != nullptr) {
        serviceThread->customerAdded(this);
    }
}


Customer::~Customer() {}


void Customer::addHostScheme(HostScheme* hostScheme) {
    hostScheme->moveToThread(thread());
    hostScheme->setParent(this);
    hostSchemeAdded(hostScheme);
}


bool Customer::removeHostScheme(HostScheme::HostSchemeId hostSchemeId) {
    bool success;

    hostSchemeMutex.lock();
    HostSchemesByHostSchemeId::iterator it = hostSchemesByHostSchemeId.find(hostSchemeId);
    if (it != hostSchemesByHostSchemeId.end()) {
        HostScheme* hostScheme = it.value();

        hostSchemesByHostSchemeId.erase(it);
        hostSchemeMutex.unlock();

        hostScheme->reportExistingMonitors(this, false);

        if (currentServiceThread != nullptr) {
            currentServiceThread->hostSchemeAboutToBeRemoved(hostScheme);
        }

        delete hostScheme;
        success = true;
    } else {
        hostSchemeMutex.unlock();
        success = false;
    }

    return success;
}


HostScheme* Customer::getHostScheme(HostScheme::HostSchemeId hostSchemeId) const {
    QMutexLocker locker(&hostSchemeMutex);
    return hostSchemesByHostSchemeId.value(hostSchemeId);
}


Monitor* Customer::getMonitor(Monitor::MonitorId monitorId) const {
    QMutexLocker locker(&monitorMutex);
    return monitorsByMonitorId.value(monitorId);
}


QList<HostScheme*> Customer::hostSchemes() const {
    return hostSchemesByHostSchemeId.values();
}


QList<Monitor*> Customer::monitors() const {
    return monitorsByMonitorId.values();
}


Customer::CustomerId Customer::customerId() const {
    return currentCustomerId;
}


bool Customer::supportsPingTesting() const {
    return currentSupportsPingTesting;
}


void Customer::setSupportsPingTesting(bool nowSupported) {
    currentSupportsPingTesting = nowSupported;
}


bool Customer::supportsSslExpirationChecking() const {
    return currentSupportsSslExpirationChecking;
}


void Customer::setSupportsSslExpirationChecking(bool nowSupported) {
    currentSupportsSslExpirationChecking = nowSupported;
}


bool Customer::supportsLatencyMeasurements() const {
    return currentSupportsLatencyMeasurements;
}


void Customer::setSupportsLatencyMeasurements(bool nowSupported) {
    currentSupportsLatencyMeasurements = nowSupported;
}


bool Customer::supportsMultiRegionTesting() const {
    return currentSupportsMultiRegionTesting;
}


void Customer::setSupportsMultiRegionTesting(bool nowSupported) {
    currentSupportsMultiRegionTesting = nowSupported;
}


unsigned Customer::pollingInterval() const {
    return currentPollingInterval;
}


void Customer::setPollingInterval(unsigned newPollingInterval) {
    currentPollingInterval = newPollingInterval;
}


bool Customer::paused() const {
    return currentlyPaused;
}


void Customer::setPaused(bool nowPaused) {
    currentlyPaused = nowPaused;
}


unsigned Customer::numberHostSchemes() const {
    QMutexLocker locker(&hostSchemeMutex);
    return static_cast<unsigned>(hostSchemesByHostSchemeId.size());
}


unsigned Customer::numberMonitors() const {
    QMutexLocker locker(&monitorMutex);
    return static_cast<unsigned>(monitorsByMonitorId.size());
}


void Customer::monitorAdded(Monitor* monitor) {
    monitorMutex.lock();
    monitorsByMonitorId.insert(monitor->monitorId(), monitor);
    monitorMutex.unlock();

    if (currentServiceThread != nullptr) {
        currentServiceThread->monitorAdded(monitor);
    }
}


void Customer::monitorAboutToBeRemoved(Monitor* monitor) {
    monitorMutex.lock();
    monitorsByMonitorId.remove(monitor->monitorId());
    monitorMutex.unlock();

    if (currentServiceThread != nullptr) {
        currentServiceThread->monitorAboutToBeRemoved(monitor);
    }
}


void Customer::hostSchemeAdded(HostScheme* hostScheme) {
    hostSchemeMutex.lock();
    hostSchemesByHostSchemeId.insert(hostScheme->hostSchemeId(), hostScheme);
    hostSchemeMutex.unlock();

    if (currentServiceThread != nullptr) {
        currentServiceThread->hostSchemeAdded(hostScheme);
    }

    hostScheme->reportExistingMonitors(this, true);
}


void Customer::hostSchemeAboutToBeRemoved(HostScheme* hostScheme) {
    HostScheme::HostSchemeId hostSchemeId = hostScheme->hostSchemeId();

    hostSchemeMutex.lock();
    hostSchemesByHostSchemeId.remove(hostSchemeId);
    hostSchemeMutex.unlock();

    hostScheme->reportExistingMonitors(this, false);

    if (currentServiceThread != nullptr) {
        currentServiceThread->hostSchemeAboutToBeRemoved(hostScheme);
    }
}


void Customer::reportExistingHostSchemesAndMonitors(HttpServiceThread* serviceThread, bool adding) {
    if (adding) {
        hostSchemeMutex.lock();

        for (  HostSchemesByHostSchemeId::const_iterator hit  = hostSchemesByHostSchemeId.constBegin(),
                                                         hend = hostSchemesByHostSchemeId.constEnd()
             ; hit != hend
             ; ++hit
            ) {
            serviceThread->hostSchemeAdded(hit.value());
        }

        hostSchemeMutex.unlock();
        monitorMutex.lock();

        for (  HostScheme::MonitorsByMonitorId::const_iterator mit  = monitorsByMonitorId.constBegin(),
                                                               mend = monitorsByMonitorId.constEnd()
             ; mit != mend
             ; ++mit
            ) {
            serviceThread->monitorAdded(mit.value());
        }

        monitorMutex.unlock();
    } else {
        hostSchemeMutex.lock();

        for (  HostSchemesByHostSchemeId::const_iterator hit  = hostSchemesByHostSchemeId.constBegin(),
                                                         hend = hostSchemesByHostSchemeId.constEnd()
             ; hit != hend
             ; ++hit
            ) {
            serviceThread->hostSchemeAboutToBeRemoved(hit.value());
        }

        hostSchemeMutex.unlock();
        monitorMutex.lock();

        for (  HostScheme::MonitorsByMonitorId::const_iterator it  = monitorsByMonitorId.constBegin(),
                                                               end = monitorsByMonitorId.constEnd()
             ; it != end
             ; ++it
            ) {
            serviceThread->monitorAboutToBeRemoved(it.value());
        }

        monitorMutex.unlock();
    }
}
