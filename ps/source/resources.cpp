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
* This header implements several functions to measure system resources.
***********************************************************************************************************************/

#include <QtGlobal>
#include <QThread>
#include <QFile>

#include <cstdlib>
#include "resources.h"

static const QByteArray memoryTotalHeader("MemTotal:");
static const QByteArray memoryAvailableHeader("MemAvailable:");

static unsigned numberCores = 0;

float cpuUtilization() {
    float  result;

    double loadAverage[3];
    int    numberSamples = getloadavg(loadAverage, 3);
    if (numberSamples > 0) {
        if (numberCores == 0) {
            numberCores = static_cast<unsigned>(QThread::idealThreadCount());
        }

        double loading = loadAverage[numberSamples - 1];
        result = loading / numberCores;
        if (result > 1.0) {
            result = 1.0;
        }
    } else {
        result = 0;
    }

    return result;
}


#if (defined(Q_OS_LINUX))

    float memoryUtilization() {
        float result;

        QFile memoryInformation("/proc/meminfo");
        bool  success = memoryInformation.open(QFile::OpenModeFlag::ReadOnly);

        QString memoryAvailable;
        QString memoryTotal;

        while (success && (memoryAvailable.isEmpty() || memoryTotal.isEmpty())) {
            QByteArray line = memoryInformation.readLine();
            if (!line.isEmpty()) {
                if (line.startsWith(memoryTotalHeader)) {
                    memoryTotal = QString::fromLocal8Bit(line);
                } else if (line.startsWith(memoryAvailableHeader)) {
                    memoryAvailable = QString::fromLocal8Bit(line);
                }
            } else {
                success = false;
            }
        }

        QStringList memoryAvailableParts = memoryAvailable.split(QChar(' '), Qt::SplitBehaviorFlags::SkipEmptyParts);
        QStringList memoryTotalParts     = memoryTotal.split(QChar(' '), Qt::SplitBehaviorFlags::SkipEmptyParts);

        if (memoryAvailableParts.size() == 3 && memoryTotalParts.size() == 3) {
            bool               okAvailable;
            bool               okTotal;
            unsigned long long available = memoryAvailableParts.at(1).toULongLong(&okAvailable);
            unsigned long long total     = memoryTotalParts.at(1).toULongLong(&okTotal);

            if (okAvailable && okTotal) {
                result = 1.0 - (static_cast<double>(available) / static_cast<double>(total));
            } else {
                result = 0;
            }
        } else {
            result = 0;
        }

        return result;
    }

#elif (defined(Q_OS_DARWIN))

    float memoryUtilization() {
        return 0;
    }

#else

    #error Unsupported platform

#endif
