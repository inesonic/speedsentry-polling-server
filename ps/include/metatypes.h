/*-*-c++-*-*************************************************************************************************************
* Copyright 2016 - 2023 Inesonic, LLC.
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
* This header defines the \ref registerMetaTypes function as well as other features used to register and manage various
* application specific metatypes.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef METATYPES_H
#define METATYPES_H

#include <QMetaType>

#include "monitor.h"
#include "host_scheme.h"
#include "data_aggregator.h"

/**
 * Function you can call during application start-up to register metatypes required by the application.
 */
void registerMetaTypes();

/*
 * Declare the various metatypes below.
 */

Q_DECLARE_METATYPE(Monitor::MonitorId)
// Q_DECLARE_METATYPE(HostScheme::HostSchemeId)
Q_DECLARE_METATYPE(HostScheme::EventType)

#endif
