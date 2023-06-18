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
* This header defines the \ref Monitor class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef MONITOR_H
#define MONITOR_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMap>
#include <QSslConfiguration>

#include <cstdint>
#include <chrono>

class QNetworkAccessManager;
class QNetworkReply;

class HostScheme;

/**
 * Class used to manage traffic related to tracking a single monitor.
 */
class Monitor:public QObject {
    Q_OBJECT

    public:
        /**
         * Value used to represent a monitor ID.  This value is used to tie this monitor back to the website and
         * customer REST API.
         */
        typedef std::uint32_t MonitorId;

        /**
         * Enumeration of supported access methods.
         */
        enum class Method {
            /**
             * Indicates access is via GET.
             */
            GET,

            /**
             * Indicates access is via HEAD.
             */
            HEAD,

            /**
             * Indicates access is via POST.
             */
            POST,

            /**
             * Indicates access is via PUT.
             */
            PUT,

            /**
             * Indicates access is via DELETE.
             */
            DELETE,

            /**
             * Indicates access is via OPTIONS.
             */
            OPTIONS,

            /**
             * Indicates access is via PATCH.
             */
            PATCH
        };

        /**
         * Enumeration of content check modes.
         */
        enum class ContentCheckMode {
            /**
             * Indicates no content checking.
             */
            NO_CHECK,

            /**
             * Indicates check for content matching (via MD5 sum or similar)
             */
            CONTENT_MATCH,

            /**
             * Indicates check for any of the supplied keywords.
             */
            ANY_KEYWORDS,

            /**
             * Indicates check for all of the supplied keywords.
             */
            ALL_KEYWORDS,

            /**
             * Indicates check for content matching, ignoring aspects that don't impact presentation or security.
             */
            SMART_CONTENT_MATCH
        };

        /**
         * Enumeration of supported POST content types.
         */
        enum class ContentType {
            /**
             * Indicates JSON content.
             */
            JSON,

            /**
             * Indicates XML content.
             */
            XML,

            /**
             * Indicates text content.
             */
            TEXT
        };

        /**
         * Enumeration of monitor status values.
         */
        enum class MonitorStatus {
            /**
             * Indicates that the status of the monitor is unknown.
             */
            UNKNOWN,

            /**
             * Indicates that the monitor appears to be working.
             */
            WORKING,

            /**
             * Indicates that the monitor appears to have failed.
             */
            FAILED
        };

        /**
         * Type used to represent a list of keywords.
         */
        typedef QByteArrayList KeywordList;

        /**
         * Type used to create a mapping of key/value pairs.
         */
        typedef QMap<QString, QString> Headers;

        /**
         * The default user agent.
         */
        static const QByteArray defaultUserAgent;

        /**
         * The default user agent header string.
         */
        static const QByteArray defaultUserAgentHeaderString;

        /**
         * The maximum transfer timemout, in mSec.
         */
        static constexpr unsigned transferTimeout = 60000;

        /**
         * The maximum allowed latency, in microseconds.
         */
        static constexpr unsigned long maximumAllowedLatencyMicroseconds = 1000 * transferTimeout;

        /**
         * Constructor.
         *
         * \param[in] monitorId            The globally unique monitor ID.
         *
         * \param[in] path                 The path under the scheme and host to be checked.
         *
         * \param[in] method               The method used to check the URL.
         *
         * \param[in] contentCheckMode     The content check mode for this monitor.
         *
         * \param[in] keywords             The list of content check keywords for this monitor.
         *
         * \param[in] contentType          The type of content to supply as part of a POST message.
         *
         * \param[in] userAgent            The user-agent string to be reported during a POST message.
         *
         * \param[in] postContent          An array of byte data to be sent as the post message.
         *
         * \param[in] hostScheme           The host/scheme that monitor is related to.
         */
        Monitor(
            MonitorId              monitorId,
            const QString&         path,
            Method                 method,
            ContentCheckMode       contentCheckMode,
            const KeywordList&     keywords,
            ContentType            contentType,
            const QString&         userAgent,
            const QByteArray&      postContent,
            HostScheme*            hostScheme = nullptr
        );

        ~Monitor() override;

        /**
         * Method you can use to determine the last status for this monitor.
         *
         * \return Returns the last recorded monitor status.
         */
        MonitorStatus monitorStatus() const;

        /**
         * Method you can use to obtain the monitor ID.
         *
         * \return Returns the monitor ID.
         */
        MonitorId monitorId() const;

        /**
         * Method you can use to obtain the host/scheme this monitor is tied to.
         *
         * \return Returns the host/scheme that owns this monitor.  A null pointer may be returned if the host/scheme
         *         no longer exists.
         */
        HostScheme* hostScheme() const;

        /**
         * Method you can use to set the host/scheme this monitor is tied to.
         *
         * \param[in] newHostScheme The new host scheme this monitor should be tied to.
         */
        void setHostScheme(HostScheme* newHostScheme);

        /**
         * Method you can use to obtain the current path under the host.  Value is as entered by the customer.
         *
         * \return Returns the path under the host and scheme for this monitor.
         */
        const QString& path() const;

        /**
         * Method you can use to change the current path under the host.
         *
         * \param[in] newPath The new path to apply to this monitor.
         */
        void setPath(const QString& newPath);

        /**
         * Method you can use to obtain the method used to access this endpoint.
         *
         * \return Returns the method used to access this endpoint.
         */
        Method method() const;

        /**
         * Method you can use to update the method used to access this endpoint.
         *
         * \param[in] newMethod The new access method.
         */
        void setMethod(Method newMethod);

        /**
         * Method you can use to determine the desired content check mode for this endpoint.
         *
         * \return Returns the endpoint content check mode.
         */
        ContentCheckMode contentCheckMode() const;

        /**
         * Method you can use to change the content check mode.
         *
         * \param[in] newContentCheckMode The new content check mode to be used.
         */
        void setContentCheckMode(ContentCheckMode newContentCheckMode);

        /**
         * Method you can use to obtain the content check keyword list.
         *
         * \return Returns the content check keyword list.
         */
        const KeywordList& keywords() const;

        /**
         * Method you can use to change the content check keyword list.
         *
         * \param[in] newKeywordList The new list of content check keywords.
         */
        void setKeywords(const KeywordList& newKeywordList);

        /**
         * Method you can use to get the content type.
         *
         * \return Returns the content type for this POST.
         */
        ContentType contentType() const;

        /**
         * Method you can use to change the content type.
         *
         * \param[in] newContentType The new content type.
         */
        void setContentType(ContentType newContentType);

        /**
         * Method you can use to obtain the user agent string.
         *
         * \return Returns the user agent string.
         */
        QString userAgent() const;

        /**
         * Method you can use to change the user agent string.
         *
         * \param[in] newUserAgent The new user agent string.
         */
        void setUserAgent(const QString& newUserAgent);

        /**
         * Method you can use to obtain the post content.
         *
         * \return Returns the post content.
         */
        const QByteArray& postContent() const;

        /**
         * Method you can use to change the post content.
         *
         * \param[in] newPostContent The new post content.
         */
        void setPostContent(const QByteArray& newPostContent);

        /**
         * Method you can use to convert a method value to a string.
         *
         * \param[in] method The method to be converted.
         *
         * \return Returns a string representation of the method.
         */
        static QString toString(Method method);

        /**
         * Method you can use to convert a string to a method value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted method value.
         */
        static Method toMethod(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a content check mode value to a string.
         *
         * \param[in] contentCheckMode The value to be converted.
         *
         * \return Returns a string representation of the content check mode.
         */
        static QString toString(ContentCheckMode contentCheckMode);

        /**
         * Method you can use to convert a string to a content check mode value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted content check mode value.
         */
        static ContentCheckMode toContentCheckMode(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a content type value to a string.
         *
         * \param[in] contentType The value to be converted.
         *
         * \return Returns a string representation of the content type value.
         */
        static QString toString(ContentType contentType);

        /**
         * Method you can use to convert a string to a content type value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted content type value.
         */
        static ContentType toContentType(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to trigger this monitor from a different thread.
         */
        void startCheckFromDifferentThread();

        /**
         * Method you can use to specify a list of standard headers to include in all HTTP requests.
         *
         * \param[in] headers A set of key/value header pairs to include in every request.
         */
        static void setDefaultHeaders(const Headers& headers);

        /**
         * Method you can use to obtain the list of default headers.
         *
         * \return Returns the list of default headers.
         */
        static Headers defaultHeaders();

    signals:
        /**
         * Signal that is emitted when a request has been made to start this monitor from another thread.
         */
        void startCheckRequested();

    public slots:
        /**
         * Slot you can trigger to start a monitor check for this monitor.
         */
        void startCheck();

        /**
         * Slot you can trigger to stop checking for this monitor.
         */
        void abort();

    private slots:
        /**
         * Slot that is triggered when a response or timeout is received.
         */
        void responseReceived();

    private:
        /**
         * Verb used to send HTTP OPTIONS command.
         */
        static const QByteArray optionsVerb;

        /**
         * Verb used to send HTTP PATCH command.
         */
        static const QByteArray patchVerb;

        /**
         * Method that is called when a valid response is received.
         *
         * \param[in] elapsedTimeNanoseconds The elapsed time, in nanoseconds.
         *
         * \param[in] sslConfiguration       The SSL configuration.
         */
        void processValidResponse(
            unsigned long long       elapsedTimeNanoseconds,
            const QSslConfiguration& sslConfiguration
        );

        /**
         * Method that is called when an error is detected.
         */
        void processErrorResponse();

        /**
         * Method that is called to check for content change.
         *
         * \param[in] receivedData The received payload data.
         */
        void checkContentChange(const QByteArray& receivedData);

        /**
         * Method that is called to check for any keyword.
         *
         * \param[in] receivedData The received payload data.
         */
        void checkAnyKeywordMatch(const QByteArray& receivedData);

        /**
         * Method that is called to check for all keywords match.
         *
         * \param[in] receivedData The received payload data.
         */
        void checkAllKeywordMatch(const QByteArray& receivedData);

        /**
         * Method that is called to check for content change using smart content checking.
         *
         * \param[in] receivedData The received payload data.
         */
        void checkContentChangeSmart(const QByteArray& receivedData);

        /**
         * String indicating text content.
         */
        static const QByteArray textPlainContentType;

        /**
         * String indicating JSON content.
         */
        static const QByteArray applicationJsonContentType;

        /**
         * String indicating XML content.
         */
        static const QByteArray applicationXmlContentType;

        /**
         * Static header key/value pairs.
         */
        static QMap<QByteArray, QByteArray> currentDefaultHeaders;

        /**
         * The monitor ID of this monitor.
         */
        MonitorId currentMonitorId;

        /**
         * The current path under the host.  Value is as entered by the customer.
         */
        QString currentPath;

        /**
         * The method used to access this endpoint.
         */
        Method currentMethod;

        /**
         * The content check mode.
         */
        ContentCheckMode currentContentCheckMode;

        /**
         * The list of content check keywords.
         */
        KeywordList currentKeywords;

        /**
         * The current content type for this POST.
         */
        ContentType currentContentType;

        /**
         * the current user agent string.
         */
        QByteArray currentUserAgent;

        /**
         * The current post content.
         */
        QByteArray currentPostContent;

        /**
         * The last recorded monitor status.
         */
        MonitorStatus currentMonitorStatus;

        /**
         * The cryptographic hash of the last received payload.
         */
        QByteArray lastHash;

        /**
         * Time point indicating the start of the current request.
         */
        QElapsedTimer elapsedTimer;

        /**
         * The timestamp when this last request was started.
         */
        unsigned long long startTimestamp;

        /**
         * The current pending network reply.
         */
        QNetworkReply* pendingReply;
};

#endif
