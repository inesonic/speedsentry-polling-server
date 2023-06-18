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
* This header defines the \ref logWrite function.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef LOG_H
#define LOG_H

#include <QString>

/**
 * Function you can use to write a log entry.
 *
 * \param[in] message The log message.
 *
 * \param[in] error   If true, the data is reporting an error.
 */
void logWrite(const QString& message, bool error = false);

#endif
