#!/usr/bin/python
#-*-python-*-##################################################################
# Copyright 2020 Inesonic, LLC
# All Rights Reserved
###############################################################################

"""
Python module that can be used to send messages via a generic webhook
mechanism.

This module will include support for posting messages only if the requests
module and related dependencies are included.

"""

###############################################################################
# Import:
#

import time
import struct
import hashlib
import hmac
import json
import base64
import requests
import cherrypy

from .rest_api_common_v1 import *

###############################################################################
# Globals:
#

DEFAULT_TIME_DELTA_SLUG = "td"
"""
The default slug to use to obtain time deltas.

"""

###############################################################################
# Class Server:
#

class Server(object):
    """
    Class that tracks information about a remote server.

    """

    def __init__(
        self,
        scheme_and_host,
        time_delta_slug = DEFAULT_TIME_DELTA_SLUG
        ):
        """
        Method that initializes the Server class.

        :param scheme_and_host:
            The server scheme and host.

        :param time_delta_slug:
            The endpoint used to determine the time delta between this
            machine and the server.

        :type scheme_and_host: str

        """

        super().__init__()

        self.__scheme_and_host = self.__fix_scheme_and_host(scheme_and_host)
        self.__time_delta_slug = self.__fix_slug(time_delta_slug)
        self.__current_time_delta = 0


    def post_message(self, slug, secret, message):
        """
        Method that will issue a request to a remote server.  If needed, the
        method will query for an updated tiem delta and perform several
        retries.

        :param slug:
            The slug to be used.

        :param secret:
            The Inesonic secret to be used to authenticate the message.

        :param message:
            A dictionary holding the message to be sent.

        :return:
            Returns a dictionary with the response or None if an error occured.

        :type slug:    str
        :type secret:  bytes or bytearray
        :type message: dict
        :rtype:        dict or None

        """

        fixed_slug = self.__fix_slug(slug)
        response = self.__post_message(fixed_slug, secret, message)
        if response is None:
            new_time_delta = self.__time_delta()
            if new_time_delta is not None:
                self.__current_time_delta = new_time_delta
                response = self.__post_message(fixed_slug, secret, message)

        return response


    def post_binary_message(self, slug, secret, message):
        """
        Method that will issue a request to a remote server using a binary
        format.  If needed, the method will query for an updated time delta and
        perform several retries.

        :param slug:
            The slug to be used.

        :param secret:
            The Inesonic secret to be used to authenticate the message.

        :param message:
            The raw binary message.

        :return:
            Returns either a dictionary or a bytes instance depending on the
            content type reported by the server.  None is returned on error.

        :type slug:    str
        :type secret:  bytes or bytearray
        :type message: bytes ir bytearray
        :rtype:        bytes, dict, or None

        """

        fixed_slug = self.__fix_slug(slug)
        (
            status_code,
            response_data,
            headers
        ) = self.__post_binary_message(
            fixed_slug,
            secret,
            message
        )

        if status_code == 401: # We got an unauthorized response.
            new_time_delta = self.__time_delta()
            if new_time_delta is not None:
                self.__current_time_delta = new_time_delta

                (
                    status_code,
                    response_data,
                    headers
                ) = self.__post_binary_message(
                    fixed_slug,
                    secret,
                    message
                )

        if status_code == 200: # OK
            content_type = None
            if 'content-type' in headers:
                content_type = headers['content-type']
            else:
                cleaned_headers = { k.lower(): v for k,v in headers.items() }
                if 'content-type' in headers:
                    content_type = headers['content-type']

            if content_type is None:
                try:
                    result = base64.b64decode(response_data)
                except:
                    result = bytes(response_data)
            else:
                content_type = content_type.lower()
                if content_type == 'application/json':
                    try:
                        result = json.loads(response_data)
                    except:
                        result = None
                else:
                    result = bytes(response_data)
        else:
            result = None

        return result


    def post_customer_message(
        self,
        slug,
        customer_identifier,
        customer_secret,
        message
        ):
        """
        Method that will issue a request to a remote server.  If needed, the
        method will query for an updated tiem delta and perform several
        retries.

        :param slug:
            The slug to be used.

        :param customer_identifier:
            The customer identifier used to identify this customer.

        :param customer_secret:
            The customer specific secret to be used to authenticate the
            message.

        :param message:
            A dictionary holding the message to be sent.

        :return:
            Returns a dictionary with the response or None if an error occured.

        :type slug:                str
        :type customer_identifier: str
        :type customer_secret:     bytes or bytearray
        :type message:             dict
        :rtype:                    dict or None

        """

        fixed_slug = self.__fix_slug(slug)
        response = self.__post_customer_message(
            fixed_slug,
            customer_identifier,
            customer_secret,
            message
        )
        if response is None:
            new_time_delta = self.__time_delta()
            if new_time_delta is not None:
                self.__current_time_delta = new_time_delta
                response = self.__post_customer_message(
                    fixed_slug,
                    customer_identifier,
                    customer_secret,
                    message
                )

        return response


    def __time_delta(self):
        """
        Function you can use to determine the system clock time delta between us
        and a remote server.

        :param website_url:
            The website URL to determine the time delta with.

        :param webhook:
            The time-delta webhook on the remote server.

        :return:
            Returns the measured time delta, in seconds, or None if not time delta
            could be determined.

        :type website_url: str
        :type webhook:     str
        :rtype:            int or None

        """

        url = "%s/%s"%(self.__scheme_and_host, self.__time_delta_slug)

        message_payload = { 'timestamp' : int(time.time()) }
        payload = json.dumps(message_payload)

        response = requests.post(
            url,
            data = payload,
            headers = {
                'User-Agent' : 'Inesonic, LLC',
                'Content-Type' : 'application/json',
                'Content-Length' : str(len(payload))
            }
        )

        if response.status_code == 200:
            try:
                json_result = json.loads(response.text)
            except:
                json_result = None

            if len(json_result) == 2                  and \
                'status' in json_result               and \
                ('time_delta' in json_result or
                 'time-delta' in json_result    )     and \
                json_result['status'].lower() == 'ok'     :
                if 'time_delta' in json_result:
                    try:
                        result = int(json_result['time_delta'])
                    except:
                        result = None
                else:
                    try:
                        result = int(json_result['time-delta'])
                    except:
                        result = None
            else:
                result = None
        else:
            result = None

        return result


    def __post_message(self, fixed_slug, secret, payload):
        """
        Function that can be used to send an arbitrary message to an Inesonic
        website via a HTTPS post method.

        :param fixed_slug:
            The fixed slug to be used.

        :param secret:
            The secret used to generate and decode the hash.

        :param payload:
            A data structure to be sent.  The data structure will be converted to
            JSON format as needed.

        :return:
            Returns the response sent by the site or None if a bad response was
            received.

        :type fixed_slug: str
        :type secret:     bytes or bytearray
        :type payload:    dict

        """

        url = "%s/%s"%(self.__scheme_and_host, fixed_slug)
        raw_message = json.dumps(payload).encode('utf-8')

        hash_time_value = int(
              (int(time.time()) + self.__current_time_delta)
            / 30
        )

        hash_time_data = struct.pack('<Q', hash_time_value)
        key = secret + hash_time_data

        raw_hash = hmac.new(
            key = key,
            msg = raw_message,
            digestmod = HASH_ALGORITHM
        ).digest()

        encoded_message = base64.b64encode(raw_message)
        encoded_hash = base64.b64encode(raw_hash)

        message_payload = {
            'data' : encoded_message.decode('utf-8'),
            'hash' : encoded_hash.decode('utf-8')
        }

        payload = json.dumps(message_payload)
        try:
            response = requests.post(
                url,
                data = payload,
                headers = {
                    'User-Agent' : 'Inesonic, LLC',
                    'Content-Type' : 'application/json',
                    'Content-Length' : str(len(payload))
                }
            )
        except requests.exceptions.ConnectionError as e:
            response = None
            cherrypy.log(
                "*** No response from %s: %s"%(url, str(e))
            )

        if response is not None and response.status_code == 200:
            try:
                result = json.loads(response.text)
            except:
                result = None
        else:
            result = None

        return result


    def __post_binary_message(self, fixed_slug, secret, payload):
        """
        Function that can be used to send an arbitrary message to an Inesonic
        website via a HTTPS post method.

        :param fixed_slug:
            The fixed slug to be used.

        :param secret:
            The secret used to generate and decode the hash.

        :param payload:
            A data structure to be sent.  The data structure will be converted to
            JSON format as needed.

        :return:
            Returns the a tuple containing the status code, raw data, and the
            response headers.

        :type fixed_slug: str
        :type secret:     bytes or bytearray
        :type payload:    dict
        :rtype:           bytes, dict, or None

        """

        url = "%s/%s"%(self.__scheme_and_host, fixed_slug)

        hash_time_value = int(
              (int(time.time()) + self.__current_time_delta)
            / 30
        )

        hash_time_data = struct.pack('<Q', hash_time_value)
        key = secret + hash_time_data

        raw_hash = hmac.new(
            key = key,
            msg = payload,
            digestmod = HASH_ALGORITHM
        ).digest()

        data_to_send = bytearray(payload)
        data_to_send.extend(raw_hash)

        response = requests.post(
            url,
            data = bytes(data_to_send),
            headers = {
                'User-Agent' : 'Inesonic, LLC',
                'Content-Type' : 'application/octet-stream',
                'Content-Length' : str(len(data_to_send))
            }
        )

        return (response.status_code, response.content, response.headers )


    def post_customer_message(
        self,
        slug,
        customer_identifier,
        customer_secret,
        message
        ):
        """
        Method that will issue a request to a remote server.  If needed, the
        method will query for an updated tiem delta and perform several
        retries.

        :param slug:
            The slug to be used.

        :param customer_identifier:
            The customer identifier used to identify this customer.

        :param customer_secret:
            The customer specific secret to be used to authenticate the
            message.

        :param message:
            A dictionary holding the message to be sent.

        :return:
            Returns a dictionary with the response or None if an error occured.

        :type slug:                str
        :type customer_identifier: str
        :type customer_secret:     bytes or bytearray
        :type message:             dict
        :rtype:                    dict or None

        """

        url = "%s/%s"%(self.__scheme_and_host, fixed_slug)
        raw_message = json.dumps(payload).encode('utf-8')

        hash_time_value = int(
              (int(time.time()) + self.__current_time_delta)
            / 30
        )

        hash_time_data = struct.pack('<Q', hash_time_value)
        key = customer_secret + hash_time_data

        raw_hash = hmac.new(
            key = key,
            msg = raw_message,
            digestmod = HASH_ALGORITHM
        ).digest()

        encoded_message = base64.b64encode(raw_message)
        encoded_hash = base64.b64encode(raw_hash)

        message_payload = {
            'cid' : customer_identifier,
            'data' : encoded_message.decode('utf-8'),
            'hash' : encoded_hash.decode('utf-8')
        }

        payload = json.dumps(message_payload)
        response = requests.post(
            url,
            data = payload,
            headers = {
                'User-Agent' : 'Inesonic, LLC',
                'Content-Type' : 'application/json',
                'Content-Length' : str(len(payload))
            }
        )

        if response.status_code == 200:
            try:
                result = json.loads(response.text)
            except:
                result = None
        else:
            result = None

        return result


    def __fix_slug(self, slug):
        """
        Method used to fix a provided slug, removing leading and trailing
        slashes.

        :param slug:
            The slug to be cleaned.

        :return:
            Returns the cleaned slug.

        :type slug: str
        :rtype:     str

        """

        if slug.startswith('/'):
            slug = slug[1:]

        if slug.endswith('/'):
            slug = slug[:-1]

        return slug


    def __fix_scheme_and_host(self, scheme_and_host):
        """
        Method used to fix a provided scheme and host, removing trailing
        slashes.

        :param scheme_and_host:
            The scheme and host to be fixed.

        :return:
            Returns the cleaned scheme and host.

        :type scheme_and_host: str
        :rtype:                str

        """

        if scheme_and_host.endswith('/'):
            scheme_and_host = scheme_and_host[:-1]

        return scheme_and_host

###############################################################################
# Functions:
#

def debug_dump_bytes(data):
    """
    Function you can use to dump an bytes or bytearray object.

    :param data:
        The data to be dumped.

    :return:
        Returns a string representation of the byte array.

    :type date: bytes or bytearray
    :rtype:     str

    """

    s = str()
    for b in data:
        if not s:
            s = "%02X"%int(b)
        else:
            s += " %02X"%int(b)

    return s

###############################################################################
# Main:
#

if __name__ == "__main__":
    import sys
    sys.stderr.write(
        "*** This module is not intended to be run as a script..\n"
    )
    exit(1)
