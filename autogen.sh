#!/bin/bash

set -xe
autoreconf --force --install --verbose -Wno-portability && ./configure "$@"
