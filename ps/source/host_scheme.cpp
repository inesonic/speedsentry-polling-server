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
* This header implements the \ref HostScheme class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QMutex>
#include <QMutexLocker>

#include <cstdint>

#include "monitor.h"
#include "host_scheme.h"
#include "customer.h"

HostScheme::HostScheme(
        HostScheme::HostSchemeId hostSchemeId,
        const QUrl&              url,
        Customer*                customer
    ):currentHostSchemeId(
        hostSchemeId
    ),currentUrl(
        url
    ) {
    if (customer != nullptr) {
        moveToThread(customer->thread());
        setParent(customer);
        customer->hostSchemeAdded(this);
    }

    currentSslExpirationTimestamp = invalidSslExpirationTimestamp;
    currentNetworkAccessManager   = nullptr;
    nonResponsiveMonitorsIterator = nonResponsiveMonitors.end();

    connect(this, &HostScheme::startCheckRequested, this, &HostScheme::serviceNextMonitor);
}


HostScheme::~HostScheme() {}


QNetworkAccessManager* HostScheme::networkAccessManager() const {
    return currentNetworkAccessManager;
}


void HostScheme::setNetworkAccessManager(QNetworkAccessManager* newNetworkAccessManager) {
    currentNetworkAccessManager = newNetworkAccessManager;
}


void HostScheme::addMonitor(Monitor* monitor) {
    monitor->moveToThread(thread());
    monitor->setParent(this);
    monitorAdded(monitor);
}


bool HostScheme::removeMonitor(Monitor::MonitorId monitorId) {
    bool success;

    monitorMutex.lock();
    MonitorsByMonitorId::iterator it = monitorsByMonitorId.find(monitorId);
    if (it != monitorsByMonitorId.end()) {
        Monitor* monitor = it.value();

        monitorsByMonitorId.erase(it);
        monitorMutex.unlock();

        Customer* customer = static_cast<Customer*>(parent());
        if (customer != nullptr) {
            customer->monitorAboutToBeRemoved(monitor);
        }

        delete monitor;
        success = true;
    } else {
        monitorMutex.unlock();
        success = false;
    }

    return success;
}


Monitor* HostScheme::getMonitor(Monitor::MonitorId monitorId) const {
    return monitorsByMonitorId.value(monitorId);
}


QList<Monitor*> HostScheme::monitors() const {
    return monitorsByMonitorId.values();
}


HostScheme::HostSchemeId HostScheme::hostSchemeId() const {
    return currentHostSchemeId;
}


Customer* HostScheme::customer() const {
    return static_cast<Customer*>(parent());
}


void HostScheme::setCustomer(Customer* newCustomer) {
    newCustomer->addHostScheme(this);
}


const QUrl& HostScheme::url() const {
    return currentUrl;
}


void HostScheme::setUrl(const QUrl& newUrl) {
    currentUrl = newUrl;
}


unsigned long long HostScheme::sslExpirationTimestamp() const {
    return currentSslExpirationTimestamp;
}


void HostScheme::setSslExpirationTimestamp(unsigned long long newSslExpirationTimestamp) {
    currentSslExpirationTimestamp = newSslExpirationTimestamp;
}


void HostScheme::startCheckFromDifferentThread() {
    emit startCheckRequested();
}


void HostScheme::serviceNextMonitor() {
    monitorMutex.lock();

    if (monitorIterator == monitorsByMonitorId.end()) {
        monitorIterator = monitorsByMonitorId.begin();
    }

    Monitor* monitor = monitorIterator.value();

    ++monitorIterator;
    if (monitorIterator == monitorsByMonitorId.end()) {
        monitorIterator = monitorsByMonitorId.begin();
    }

    if (!nonResponsiveMonitors.isEmpty()) {
        if (nonResponsiveMonitorsIterator == nonResponsiveMonitors.end()) {
            nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
        }

        Monitor* nonResponsiveMonitor = *nonResponsiveMonitorsIterator;

        ++nonResponsiveMonitorsIterator;
        if (nonResponsiveMonitorsIterator == nonResponsiveMonitors.end()) {
            nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
        }

        monitorMutex.unlock();
        monitor->startCheck();
        if (monitor != nonResponsiveMonitor) {
            nonResponsiveMonitor->startCheck();
        }
    } else {
        monitorMutex.unlock();
        monitor->startCheck();
    }
}


void HostScheme::monitorNonResponsive(Monitor* monitor) {
    QMutexLocker locker(&monitorMutex);
    if (nonResponsiveMonitors.isEmpty()) {
        nonResponsiveMonitors.insert(monitor);
        nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
    } else {
        nonResponsiveMonitors.insert(monitor);
    }
}


void HostScheme::monitorNowResponsive(Monitor *monitor) {
    monitorMutex.lock();
    if (nonResponsiveMonitorsIterator != nonResponsiveMonitors.end() && *nonResponsiveMonitorsIterator == monitor) {
        nonResponsiveMonitorsIterator = nonResponsiveMonitors.erase(nonResponsiveMonitorsIterator);
        if (nonResponsiveMonitorsIterator == nonResponsiveMonitors.end() && !nonResponsiveMonitors.isEmpty()) {
            nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
        }
    } else {
        nonResponsiveMonitors.remove(monitor);
    }

    if (!nonResponsiveMonitors.isEmpty()) {
        Monitor* nextMonitorToTest = *nonResponsiveMonitors.begin();

        ++nonResponsiveMonitorsIterator;
        if (nonResponsiveMonitorsIterator == nonResponsiveMonitors.end()) {
            nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
        }

        monitorMutex.unlock();

        nextMonitorToTest->startCheck();
    } else {
        monitorMutex.unlock();
    }
}


void HostScheme::monitorAdded(Monitor* monitor) {
    monitorMutex.lock();

    bool isFirstMonitor = monitorsByMonitorId.isEmpty();
    monitorsByMonitorId.insert(monitor->monitorId(), monitor);

    bool isFirstNonResponsiveMonitor = nonResponsiveMonitors.isEmpty();
    nonResponsiveMonitors.insert(monitor);

    if (isFirstMonitor) {
        monitorIterator = monitorsByMonitorId.begin();
    }

    if (isFirstNonResponsiveMonitor) {
        nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
    }

    monitorMutex.unlock();

    Customer* customer = static_cast<Customer*>(parent());
    if (customer != nullptr) {
        customer->monitorAdded(monitor);
    }
}


void HostScheme::monitorAboutToBeRemoved(Monitor* monitor) {
    Monitor::MonitorId monitorId = monitor->monitorId();

    monitorMutex.lock();

    MonitorsByMonitorId::iterator it = monitorsByMonitorId.find(monitorId);
    if (it == monitorIterator) {
        monitorIterator = monitorsByMonitorId.erase(it);
        if (monitorIterator == monitorsByMonitorId.end()) {
            monitorIterator = monitorsByMonitorId.begin();
        }
    } else {
        monitorsByMonitorId.erase(it);
    }

    if (nonResponsiveMonitorsIterator != nonResponsiveMonitors.end() && *nonResponsiveMonitorsIterator == monitor) {
        nonResponsiveMonitorsIterator = nonResponsiveMonitors.erase(nonResponsiveMonitorsIterator);
        if (nonResponsiveMonitorsIterator == nonResponsiveMonitors.end() && !nonResponsiveMonitors.isEmpty()) {
            nonResponsiveMonitorsIterator = nonResponsiveMonitors.begin();
        }
    } else {
        nonResponsiveMonitors.remove(monitor);
    }

    monitorMutex.unlock();

    Customer* customer = static_cast<Customer*>(parent());
    if (customer != nullptr) {
        customer->monitorAboutToBeRemoved(monitor);
    }
}


void HostScheme::reportExistingMonitors(Customer* customer, bool adding) {
    QMutexLocker locker(&monitorMutex);

    if (adding) {
        for (  MonitorsByMonitorId::const_iterator it  = monitorsByMonitorId.constBegin(),
                                                   end = monitorsByMonitorId.constEnd()
             ; it != end
             ; ++it
            ) {
            customer->monitorAdded(it.value());
        }
    } else {
        for (  MonitorsByMonitorId::const_iterator it  = monitorsByMonitorId.constBegin(),
                                                   end = monitorsByMonitorId.constEnd()
             ; it != end
             ; ++it
            ) {
            customer->monitorAboutToBeRemoved(it.value());
        }
    }
}
