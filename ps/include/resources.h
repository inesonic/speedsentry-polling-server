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
* This header defines several functions to measure system resources.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef RESOURCES_H
#define RESOURCES_H

/**
 * Function that returns an estimate of the system CPU utilization.
 *
 * \return Returns an estimate of the fractional CPU utilization of the system.
 */
float cpuUtilization();

/**
 * Function that returns an estimate of the memory utilization.
 *
 * \return Returns a estimate of the fraction of the total memory used.
 */
float memoryUtilization();

#endif
