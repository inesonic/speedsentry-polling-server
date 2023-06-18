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
* This file contains the main entry point for the Zoran polling server.
***********************************************************************************************************************/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "log.h"
#include "metatypes.h"
#include "ps.h"

int main(int argumentCount, char* argumentValues[]) {
    int exitStatus = 0;

    QCoreApplication application(argumentCount, argumentValues);
    QCoreApplication::setApplicationName("Inesonic Polling Server");
    QCoreApplication::setApplicationVersion("1.0");

    registerMetaTypes();

    if (argumentCount == 2) {
        QString configurationFilename = QString::fromLocal8Bit(argumentValues[1]);
        PollingServer pollingServer(configurationFilename);
        logWrite(QString("Polling server started."), false);
        exitStatus = application.exec();
    } else {
        logWrite(QString("Invalid command line.  Include path to the configuration file."), true);
        exitStatus = 1;
    }

    return exitStatus;
}
