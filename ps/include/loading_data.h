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
* This header defines the \ref LoadingData class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef LOADING_H
#define LOADING_H

/**
 * Trivial class that tracks loading data for our polling server.
 */
class LoadingData {
    public:
        /**
         * The minimum number of polled monitors before we have acceptable timing data.
         */
        static constexpr unsigned long minimumAcceptablePolledMonitors = 1000;

        constexpr LoadingData():
            currentNumberPolledHostSchemes(0),
            currentNumberMissedTimingMarks(0),
            currentAverageTimingError(0) {}

        /**
         * Constructor
         *
         * \param[in] numberPolledHostSchemes The number of polled host/schemes in this snapshot.
         *
         * \param[in] numberMissedTimingMarks The number of times we've missed a timing mark.
         *
         * \param[in] averageTimingError      The average induced timing error.
         */
        constexpr LoadingData(
                unsigned long numberPolledHostSchemes,
                unsigned long numberMissedTimingMarks,
                double        averageTimingError
            ):currentNumberPolledHostSchemes(
                numberPolledHostSchemes
            ),currentNumberMissedTimingMarks(
                numberMissedTimingMarks
            ),currentAverageTimingError(
                averageTimingError
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        constexpr LoadingData(
                const LoadingData& other
            ):currentNumberPolledHostSchemes(
                other.currentNumberPolledHostSchemes
            ),currentNumberMissedTimingMarks(
                other.currentNumberMissedTimingMarks
            ),currentAverageTimingError(
                other.currentAverageTimingError
            ) {}

        ~LoadingData() = default;

        /**
         * Method you can use to get the current number of polled host/schemes.
         *
         * \return Returns the current number of polled host/schemes.
         */
        inline unsigned long numberPolledHostSchemes() const {
            return currentNumberPolledHostSchemes;
        }

        /**
         * Method you can use to get the current number of missed timing marks.
         *
         * \return Returns the current number of missed timing marks.
         */
        inline unsigned long numberMissedTimingMarks() const {
            return currentNumberMissedTimingMarks;
        }

        /**
         * Method you can use to get the average timing error.
         *
         * \return Returns the average timing error.  A negative value indicates that insufficient data was collected.
         */
        inline double averageTimingError() const {
            return currentAverageTimingError;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline LoadingData& operator=(const LoadingData& other) {
            currentNumberPolledHostSchemes = other.currentNumberPolledHostSchemes;
            currentNumberMissedTimingMarks = other.currentNumberMissedTimingMarks;
            currentAverageTimingError      = other.currentAverageTimingError;

            return *this;
        }

    private:
        /**
         * the current number of polled host/schemes
         */
        unsigned long currentNumberPolledHostSchemes;

        /**
         * The current number of missed timing marks.
         */
        unsigned long currentNumberMissedTimingMarks;

        /**
         * The average timing error.
         */
        double currentAverageTimingError;
};

#endif
