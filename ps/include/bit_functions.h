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
* This header defines several useful bit manipulation function.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef BIT_FUNCTIONS_H
#define BIT_FUNCTIONS_H

#include <cstdint>

/**
 * Function you can use to reverse bit ordering.
 *
 * \param[in] v The value to be reversed.
 *
 * \return Returns a bit reversed version of this byte.
 */
inline std::uint8_t bitReverse8(std::uint8_t v) {
                                               // v = abcdefgh
    v = ((v >> 1) & 0x55) | ((v << 1) & 0xAA); // v = badc fehg
    v = ((v >> 2) & 0x33) | ((v << 2) & 0xCC); // v = dcba hgfe
    v = ((v >> 4) & 0x0F) | ((v << 4) & 0xF0); // v = hgfe dcba

    return v;
}

/**
 * Function you can use to reverse bit ordering.
 *
 * \param[in] v The value to be reversed.
 *
 * \return Returns a bit reversed version of this byte.
 */
inline std::uint16_t bitReverse16(std::uint16_t v) {
    v = ((v >> 1) & 0x5555) | ((v << 1) & 0xAAAA);
    v = ((v >> 2) & 0x3333) | ((v << 2) & 0xCCCC);
    v = ((v >> 4) & 0x0F0F) | ((v << 4) & 0xF0F0);
    v = ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);

    return v;
}

/**
 * Function you can use to reverse bit ordering.
 *
 * \param[in] v The value to be reversed.
 *
 * \return Returns a bit reversed version of this byte.
 */
inline std::uint32_t bitReverse32(std::uint32_t v) {
    v = ((v >>  1) & 0x55555555UL) | ((v <<  1) & 0xAAAAAAAAUL);
    v = ((v >>  2) & 0x33333333UL) | ((v <<  2) & 0xCCCCCCCCUL);
    v = ((v >>  4) & 0x0F0F0F0FUL) | ((v <<  4) & 0xF0F0F0F0UL);
    v = ((v >>  8) & 0x00FF00FFUL) | ((v <<  8) & 0xFF00FF00UL);
    v = ((v >> 16) & 0x0000FFFFUL) | ((v << 16) & 0xFFFF0000UL);

    return v;
}

#endif
