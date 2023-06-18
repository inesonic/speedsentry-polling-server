##-*-makefile-*-########################################################################################################
# Copyright 2021 - 2023 Inesonic, LLC
#
# GNU Public License, Version 3:
#   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
#   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
#   version.
#   
#   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
#   details.
#   
#   You should have received a copy of the GNU General Public License along with this program.  If not, see
#   <https://www.gnu.org/licenses/>.
########################################################################################################################

########################################################################################################################
# Basic build characteristics
#

TEMPLATE = app
QT += core network
CONFIG += console
CONFIG += c++14

########################################################################################################################
# Headers
#

INCLUDEPATH += include
HEADERS = include/metatypes.h \
          include/log.h \
          include/bit_functions.h \
          include/ps.h \
          include/resources.h \
          include/loading_data.h \
          include/monitor.h \
          include/host_scheme.h \
          include/customer.h \
          include/service_thread.h \
          include/service_thread_tracker.h \
          include/host_scheme_timer.h \
          include/http_service_thread.h \
          include/ping_service_thread.h \
          include/event_reporter.h \
          include/certificate_reporter.h \
          include/data_aggregator.h \
          include/inbound_rest_api.h \

########################################################################################################################
# Source files
#

SOURCES = source/main.cpp \
          source/log.cpp \
          source/metatypes.cpp \
          source/ps.cpp \
          source/resources.cpp \
          source/monitor.cpp \
          source/host_scheme.cpp \
          source/customer.cpp \
          source/service_thread.cpp \
          source/service_thread_tracker.cpp \
          source/host_scheme_timer.cpp \
          source/http_service_thread.cpp \
          source/ping_service_thread.cpp \
          source/ping_service_thread_private.cpp \
          source/event_reporter.cpp \
          source/certificate_reporter.cpp \
          source/data_aggregator.cpp \
          source/inbound_rest_api.cpp \

########################################################################################################################
# Private headers
#

INCLUDEPATH += source
HEADERS += source/ping_service_thread_private.h \

########################################################################################################################
# Libraries
#

INCLUDEPATH += $${INEREST_API_IN_V1_INCLUDE}
INCLUDEPATH += $${INEREST_API_OUT_V1_INCLUDE}
INCLUDEPATH += $${INECRYPTO_INCLUDE}
INCLUDEPATH += $${INEXTEA_INCLUDE}
INCLUDEPATH += $${INEHTML_SCRUBBER_INCLUDE}

LIBS += -L$${INEREST_API_IN_V1_LIBDIR} -linerest_api_in_v1
LIBS += -L$${INEREST_API_OUT_V1_LIBDIR} -linerest_api_out_v1
LIBS += -L$${INECRYPTO_LIBDIR} -linecrypto
LIBS += -L$${INEXTEA_LIBDIR} -linextea
LIBS += -L$${INEHTML_SCRUBBER_LIBDIR} -linehtml_scrubber

########################################################################################################################
# Locate build intermediate and output products
#

TARGET = ps

CONFIG(debug, debug|release) {
    unix:DESTDIR = build/debug
    win32:DESTDIR = build/Debug
} else {
    unix:DESTDIR = build/release
    win32:DESTDIR = build/Release
}

OBJECTS_DIR = $${DESTDIR}/objects
MOC_DIR = $${DESTDIR}/moc
RCC_DIR = $${DESTDIR}/rcc
UI_DIR = $${DESTDIR}/ui
