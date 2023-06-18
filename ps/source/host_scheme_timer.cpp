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
* This header implements the \ref HostSchemeTimer class.
***********************************************************************************************************************/

#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QHash>
#include <QMutexLocker>
#include <QMutex>

#include <cstdint>

#include "log.h"
#include "monitor.h"
#include "bit_functions.h"
#include "loading_data.h"
#include "host_scheme_timer.h"

HostSchemeTimer::HostSchemeTimer(
        bool     multiRegion,
        int      period,
        unsigned regionIndex,
        unsigned numberRegions,
        bool     startActive,
        QObject* parent
    ):QObject(
        parent
    ),currentMultiRegion(
        multiRegion
    ),currentAggregatePeriodSeconds(
        period
    ),currentPeriodMilliseconds(
        multiRegion ? (1000 * period * numberRegions) : (1000 * period)
    ),currentRegionIndex(
        regionIndex
    ),currentNumberRegions(
        numberRegions
    ) {
    monitorIterator = monitorsByMonitorId.end();
    serviceTimer    = new QTimer(this);
    serviceTimer->setSingleShot(true);

    numberMissedTimingWindows          = 0;
    sumMillisecondsMissedTimingMarks   = 0;
    nextTimingMarkReset                = 0;
    forceTimerResync                   = false;

    if (numberRegions == 0) {
        regionTimeOffsetMilliseconds = 0;
    } else {
        regionTimeOffsetMilliseconds = (currentPeriodMilliseconds * regionIndex) / numberRegions;
    }

    logWrite(
        QString("Adjusting Host/Scheme Timer.  Region %1/%2, Period %3 mSec, Offset %4 mSec")
        .arg(regionIndex)
        .arg(numberRegions)
        .arg(currentPeriodMilliseconds)
        .arg(regionTimeOffsetMilliseconds),
        false
    );

    connect(serviceTimer, &QTimer::timeout, this, &HostSchemeTimer::startNextHostScheme);
    connect(this, &HostSchemeTimer::start, this, &HostSchemeTimer::startTimer);
    connect(this, &HostSchemeTimer::stop, serviceTimer, &QTimer::stop);

    if (startActive) {
        startTimer();
    }
}


HostSchemeTimer::~HostSchemeTimer() {}


float HostSchemeTimer::monitorsPerSecond() const {
    return (1000.0 * hostSchemesByTimeOrder.size()) / currentPeriodMilliseconds;
}


LoadingData HostSchemeTimer::loadingData() const {
    QMutexLocker locker(&hostSchemeMutex);
    return currentLoadingData;
}


void HostSchemeTimer::addHostScheme(HostScheme* hostScheme) {
    QMutexLocker locker(&hostSchemeMutex);

    hostSchemesByTimeOrder.insert(bitReverse32(hostScheme->hostSchemeId()), QPointer<HostScheme>(hostScheme));

    if (!serviceTimer->isActive()) {
        hostSchemeIterator = hostSchemesByTimeOrder.begin();
        emit start();
    }
}


bool HostSchemeTimer::removeHostScheme(HostScheme::HostSchemeId hostSchemeId) {
    bool success;

    QMutexLocker locker(&hostSchemeMutex);
    HostSchemesByTimeOrder::iterator it = hostSchemesByTimeOrder.find(bitReverse32(hostSchemeId));
    if (it != hostSchemesByTimeOrder.end()) {
        if (hostSchemeIterator == it) {
            hostSchemeIterator = hostSchemesByTimeOrder.erase(it);
        } else {
            hostSchemesByTimeOrder.erase(it);
        }

        success = true;
    } else {
        success = false;
    }

    return success;
}


QPointer<HostScheme> HostSchemeTimer::getHostScheme(HostScheme::HostSchemeId hostSchemeId) const {
    QMutexLocker locker(&hostSchemeMutex);
    return hostSchemesByTimeOrder.value(bitReverse32(hostSchemeId));
}


void HostSchemeTimer::updateRegionData(unsigned regionIndex, unsigned numberRegions) {
    currentRegionIndex   = regionIndex;
    currentNumberRegions = numberRegions;

    currentPeriodMilliseconds = (
          1000
        * (currentMultiRegion ? currentAggregatePeriodSeconds * numberRegions : currentAggregatePeriodSeconds)
    );

    if (numberRegions == 0) {
        regionTimeOffsetMilliseconds = 0;
    } else {
        regionTimeOffsetMilliseconds = (currentPeriodMilliseconds * regionIndex) / numberRegions;
    }

    logWrite(
        QString("Adjusting Host/Scheme Timer.  Region %1/%2, Period %3 mSec, Offset %4 mSec")
        .arg(regionIndex)
        .arg(numberRegions)
        .arg(currentPeriodMilliseconds)
        .arg(regionTimeOffsetMilliseconds),
        false
    );

    hostSchemeMutex.lock();
    if (serviceTimer->isActive()) {
        forceTimerResync = true;
    }

    bool workAvailable = !hostSchemesByTimeOrder.isEmpty();
    hostSchemeMutex.unlock();

    if (workAvailable) {
        emit start();
    }
}


void HostSchemeTimer::goInactive() {
    emit stop();
}


void HostSchemeTimer::goActive() {
    emit start();
}


void HostSchemeTimer::startNextHostScheme() {
    hostSchemeMutex.lock();
    HostSchemesByTimeOrder::iterator end               = hostSchemesByTimeOrder.end();
    unsigned                         numberHostSchemes = static_cast<unsigned>(hostSchemesByTimeOrder.size());

    // There's a corner case where we have end up pointing to the end of the host/scheme list.  This can happen
    // either when we deleted the last host/scheme in the map or when the map goes empty.  If that happens, we force
    // our timing to resync or we go idle depending on whether we have any host/schemes to be serviced.

    if (hostSchemeIterator == end || forceTimerResync) {
        forceTimerResync = false;
        if (numberHostSchemes > 0) {
            hostSchemeMutex.unlock();
            restartTimingCycle();
        } else {
            hostSchemeMutex.unlock();
        }
    } else {
        QPointer<HostScheme> hostScheme = hostSchemeIterator.value();

        ++hostSchemeIterator;
        if (hostSchemeIterator == end) {
            hostSchemeMutex.unlock();
            restartTimingCycle();
        } else {
            hostSchemeMutex.unlock();
            scheduleNextHostScheme();
        }

        hostScheme->serviceNextMonitor();
    }
}


void HostSchemeTimer::startTimer() {
    if (currentNumberRegions > 0 && !hostSchemesByTimeOrder.isEmpty()) {
        numberMissedTimingWindows          = 0;
        sumMillisecondsMissedTimingMarks   = 0;
        nextTimingMarkReset                = QDateTime::currentMSecsSinceEpoch() + missedTimingMarkResetInterval;

        restartTimingCycle();
    }
}


void HostSchemeTimer::restartTimingCycle() {
    hostSchemeIterator = hostSchemesByTimeOrder.begin();

    unsigned long long currentTime = QDateTime::currentMSecsSinceEpoch();
    unsigned long long cycleIndex  = currentTime / currentPeriodMilliseconds;

    pollingCycleStartTime = currentPeriodMilliseconds * (cycleIndex + 1) + regionTimeOffsetMilliseconds;

    scheduleNextHostScheme();
}


void HostSchemeTimer::scheduleNextHostScheme() {
    double             timeFraction = static_cast<double>(hostSchemeIterator.key()) / 4294967296.0;
    unsigned long long timeOffset   = static_cast<unsigned long>(currentPeriodMilliseconds * timeFraction + 0.5);
    unsigned long long nextEvent    = pollingCycleStartTime + timeOffset;
    unsigned long long currentTime  = QDateTime::currentMSecsSinceEpoch();

    if (nextEvent > currentTime) {
        serviceTimer->start(nextEvent - currentTime);
    } else {
        unsigned long long missedBy = currentTime - nextEvent;
        if (missedBy > 1) {
            ++numberMissedTimingWindows;
            sumMillisecondsMissedTimingMarks += missedBy;
        }

        serviceTimer->start(0);
    }

    if (currentTime > nextTimingMarkReset) {
        QMutexLocker locker(&hostSchemeMutex);
        double averageMissedTimingMarks;
        if (numberMissedTimingWindows > 0) {
            averageMissedTimingMarks = sumMillisecondsMissedTimingMarks / (1000.0 * numberMissedTimingWindows);
        } else {
            averageMissedTimingMarks = 0;
        }

        currentLoadingData = LoadingData(
            static_cast<unsigned long>(hostSchemesByTimeOrder.size()),
            numberMissedTimingWindows,
            averageMissedTimingMarks
        );

        numberMissedTimingWindows        = 0;
        sumMillisecondsMissedTimingMarks = 0;
        nextTimingMarkReset              += missedTimingMarkResetInterval;
    }
}
