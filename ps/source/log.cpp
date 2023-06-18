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
* This header implements the \ref Monitor class.
***********************************************************************************************************************/

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

#include <iostream>

#include "log.h"

QMutex loggingMutex;

void logWrite(const QString& message, bool error) {
    QMutexLocker mutexLocker(&loggingMutex);

    QString dateTime = QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate);

    if (error) {
        QString logEntry = QString("%1: *** %2").arg(dateTime, message);
        std::cerr << logEntry.toLocal8Bit().data() << std::endl;
    } else {
        QString logEntry = QString("%1: %2").arg(dateTime, message);
        std::cout << logEntry.toLocal8Bit().data() << std::endl;

    }
}
