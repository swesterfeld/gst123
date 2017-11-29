#!/bin/bash

echo "Running: autoreconf -i && ./configure $@"
autoreconf -i -Wno-portability && ./configure "$@"
