#!/usr/bin/python
#-*-python-*-##################################################################
# Copyright 2020 Inesonic, LLC
# All Rights Reserved
###############################################################################

"""
Python module that holds common resources used by the V1 version of the REST
API.

"""

###############################################################################
# Import:
#

import hashlib

###############################################################################
# Globals:
#

HASH_ALGORITHM = hashlib.sha256
"""
The hashing algorithm to be used to generate the key.

"""

HASH_BLOCK_SIZE = 64
"""
The block length to use for the HMAC.

"""

SECRET_LENGTH = HASH_BLOCK_SIZE - 8
"""
The required secret length.  Value is selected to provide good security while
also minimizing computation time for the selected hash algorithm.

"""

###############################################################################
# Main:
#

if __name__ == "__main__":
    import sys
    sys.stderr.write(
        "*** This module is not intended to be run as a script..\n"
    )
    exit(1)
