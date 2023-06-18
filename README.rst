===================================
Inesonic SpeedSentry Polling Server
===================================
The Inesonic SpeedSentry Polling Server project provides a service you can
deploy in conjunction with the SpeedSentry pinger and SpeedSentry DBC to
monitor one or more remote websites or REST API endpoints.  This project
provides:

* A server that periodically performs HTTP GET or POST requests on remote
  endpoints.

* Optionally generates hashes of the received data, comparing the hash against
  previous values.

* Optionally runs a heuristic algorithm on the received data to remove nonces
  and other content that can be expected to change between HTTP GET requests.

* Controls a SpeedSentry pinger daemon to also issue ICMP echo requests to a
  remote server, the pinger daemon will report if more than a specified number
  of requests did not result in an echo response.

* Obtains settings on start-up from a SpeedSentry DBC server.  No local
  persistent storage is needed allowing the deployed polling servers to be
  very small.  Note that the DBC can support multiple polling servers in a
  given geographic region and will evenly divide tasks across polling servers.

* Polling servers also periodically report their CPU and network utilization
  to allow monitoring of the system and to determine when scaling is needed.

The polling server requires the Qt libraries and employs the QMAKE build
environment.  The polling server relies on the following projects:

* https://github.com/inesonic/inecrypto.git

* https://github.com/inesonic/inerest_api_in_v1.git

* https://github.com/inesonic/inerest_api_out_v1.git

* https://github.com/inesonic/inehtml_scrubber.git

* https://github.com/inesonic/speedentry-pinger.git

You must set the following QMAKE variables on the QMAKE command line:

* INECRYPTO_INCLUDE
  
* INECRYPTO_LIBDIR
  
* INEREST_API_IN_V1_INCLUDE
  
* INEREST_API_IN_V1_LIBDIR
  
* INEREST_API_OUT_V1_INCLUDE
  
* INEREST_API_OUT_V1_LIBDIR
  
* INEHTML_SCRUBBER_INCLUDE
  
* INEHTML_SCRUBBER_LIBDIR

The project also includes a small Python based command line tool you can use
to perform direct control of a polling server.


Licensing
=========
This code is licensed under the GNU Public License, version 3.0.
