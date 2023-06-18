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
* This header defines the \ref ServiceThread class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef SERVICE_THREAD_H
#define SERVICE_THREAD_H

#include <QThread>
#include <QSharedPointer>
#include <QHash>

#include <cstdint>

#include <customer.h>

/**
 * Class that manages an independent monitor service thread.
 */
class ServiceThread:public QThread {
    Q_OBJECT

    friend class Customer;

    public:
        /**
         * Enumeration of thread status values.
         */
        enum class ThreadStatus {
            /**
             * Indicates the thread is inactive.
             */
            INACTIVE,

            /**
             * Indicates the thread is active.
             */
            ACTIVE,

            /**
             * Indicates the thread is in the process of going inactive.
             */
            GOING_INACTIVE
        };

        /**
         * Constructor.
         *
         * \param[in] parent Pointer to the parent object.
         */
        ServiceThread(QObject* parent = nullptr);

        ~ServiceThread() override;

        /**
         * Method you can use to determine the current state of this thread.
         *
         * \return Returns the current thread state.
         */
        ThreadStatus threadStatus() const;

    public slots:
        /**
         * Slot you can trigger to command this thread to go inactive.
         */
        virtual void goInactive();

        /**
         * Slot you can trigger to command this thread to go active.
         */
        virtual void goActive();

    private:
        /**
         * Method that indicates if this thread should go inactive now.
         *
         * \return Returns true if a shutdown has been requested.
         */
        inline bool shutdownRequested() const {
            return goingInactive;
        }

    private:
        /**
         * Flag indicating if this thread is attempting to go inactive.
         */
        bool goingInactive;
};

#endif
