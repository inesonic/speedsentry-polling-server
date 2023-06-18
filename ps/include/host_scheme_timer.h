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
* This header defines the \ref HostSchemeTimer class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef HOST_SCHEME_TIMER_H
#define HOST_SCHEME_TIMER_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QPointer>

#include <cstdint>

#include "monitor.h"
#include "loading_data.h"
#include "host_scheme.h"
#include "customer.h"

/**
 * Class that manages timing of checks by host/scheme.
 */
class HostSchemeTimer:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor.
         *
         * \param[in] multiRegion   If true, then timing should be adjusted based on the number of regions. If false,
         *                          then this customer is being checked only from this single polling server.
         *
         * \param[in] period        The per-customer host/scheme service period, in seconds.
         *
         * \param[in] regionIndex   This region's index.
         *
         * \param[in] numberRegions The number of regions.
         *
         * \param[in] startActive   If true, then we're active. if false, then we're inactive.
         *
         * \param[in] parent        Pointer to the parent object.
         */
        HostSchemeTimer(
            bool     multiRegion,
            int      period,
            unsigned regionIndex,
            unsigned numberRegions,
            bool     startActive,
            QObject* parent = nullptr
        );

        ~HostSchemeTimer() override;

        /**
         * Method you can use to obtain a service metric for this timer.  Returned value is in monitors per
         * second.
         *
         * \return Returns the current monitor service metric.
         */
        float monitorsPerSecond() const;

        /**
         * Method you can use to obtain the current loading data for this host/scheme timer.
         *
         * \return Returns the loading data for this host/scheme timer.
         */
        LoadingData loadingData() const;

        /**
         * Method you can use to add a host/scheme.  Host schemes are tracked by host scheme ID and also by customer
         * ID.  If a host/scheme is registered with the same host/scheme ID, the exsting host scheme will be replaced.
         *
         * Note that this class does *not* take ownership of the host/scheme.
         *
         * \param[in] hostScheme The host/scheme to be added.
         */
        void addHostScheme(HostScheme* hostScheme);

        /**
         * Method you can use to remove a host/scheme.
         *
         * \param[in] hostSchemeId The ID of the host/scheme to be removed.
         *
         * \return Returns true on success.  Returns false if the moni is not tied to this host scheme instance.
         */
        bool removeHostScheme(HostScheme::HostSchemeId hostSchemeId);

        /**
         * Method you can use to obtain a pointer to a host/scheme by host/scheme ID.
         *
         * \param[in] hostSchemeId The ID of the host/scheme of interest.
         *
         * \return Returns the desired host/scheme.  A null pointer is returned if the host/scheme is not registered
         *         with this timer.
         */
        QPointer<HostScheme> getHostScheme(HostScheme::HostSchemeId hostSchemeId) const;

    public slots:
        /**
         * Slot you can use to update the region settings.  Be sure to trigger this slot before setting this
         * service thread as active.  This method will also cause this timer to go active.
         *
         * \param[in] regionIndex   The zero based region index for the region we're in.
         *
         * \param[in] numberRegions The number of regions we're in.
         */
        void updateRegionData(unsigned regionIndex, unsigned numberRegions);

        /**
         * Slot you can trigger to command this timer to go inactive.
         */
        void goInactive();

        /**
         * Slot you can trigger to command this timer to go active.
         */
        void goActive();

    signals:
        /**
         * Signal that is emitted to adjust the timer setting.  We use a signal because the QTimer class does not allow
         * start or stop to be called across threads.
         */
        void start();

        /**
         * Signal that is emitted to stop the timer.
         */
        void stop();

    protected slots:
        /**
         * Method that is triggered at period intervals to start the next check.
         */
        void startNextHostScheme();

    private slots:
        /**
         * Method that starts the timers.
         */
        void startTimer();

    private:
        /**
         * Period used to look for missed timing marks, in milliseconds.
         */
        static const unsigned missedTimingMarkResetInterval = 2 * 1000 * 3600;

        /**
         * Type used to track host/schemes in time order.
         */
        typedef QMap<std::uint32_t, QPointer<HostScheme>> HostSchemesByTimeOrder;

        /**
         * Method that restarts the timing cycle.
         */
        void restartTimingCycle();

        /**
         * Method that schedules the next host/scheme.
         */
        void scheduleNextHostScheme();

        /**
         * Timer used to trigger service operations.
         */
        QTimer* serviceTimer;

        /**
         * Flag indicating that the timer settings need to be recalculated.
         */
        bool forceTimerResync;

        /**
         * Flag indicating if this is a multi-region test.
         */
        bool currentMultiRegion;

        /**
         * Value holding the current service period, in seconds.  This value ignores the number of regions.
         */
        unsigned currentAggregatePeriodSeconds;

        /**
         * The service period for this collection of host/schemes.  This value takes into account number of regions.
         */
        unsigned long currentPeriodMilliseconds;

        /**
         * The current region index we're operating in.
         */
        unsigned currentRegionIndex;

        /**
         * The current number of known regions.
         */
        unsigned currentNumberRegions;

        /**
         * The time offset to apply for this region, in milliseconds.
         */
        unsigned long regionTimeOffsetMilliseconds;

        /**
         * The time of thie current polling cycle to start, in milliseconds since the Unix epoch.
         */
        unsigned long long pollingCycleStartTime;

        /**
         * The time for the next service to be triggered, in milliseconds since the Unix epoch.
         */
        unsigned long long nextMonitorStart;

        /**
         * The time when we should reset our missed timing mark calculation.
         */
        unsigned long long nextTimingMarkReset;

        /**
         * Total number of polled hostSchemes.
         */
        unsigned long numberPolledHostSchemes;

        /**
         * The number of times we've missed our timing window.
         */
        unsigned long numberMissedTimingWindows;

        /**
         * Sum of the amount of time we missed our timing marks, in milliseconds.
         */
        unsigned long long sumMillisecondsMissedTimingMarks;

        /**
         * The last reported timing mark data.
         */
        LoadingData currentLoadingData;

        /**
         * Mutex used to protect our host/scheme hash table.
         */
        mutable QMutex hostSchemeMutex;

        /**
         * Hash table of host schemes by host/scheme ID.
         */
        HostSchemesByTimeOrder hostSchemesByTimeOrder;

        /**
         * Iterator used to walk our host/schemes in time order.
         */
        HostSchemesByTimeOrder::iterator hostSchemeIterator;

        /**
         * Hash table of monitors under this host/scheme.
         */
        HostScheme::MonitorsByMonitorId monitorsByMonitorId;

        /**
         * Current iterator into the monitor hash table.
         */
        HostScheme::MonitorsByMonitorId::iterator monitorIterator;
};

#endif
