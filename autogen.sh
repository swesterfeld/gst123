#!/bin/bash

echo "Ensure: ChangeLog"
test -e ChangeLog || TZ=GMT0 touch ChangeLog -t 190112132145.52 # automake *requires* ChangeLog

echo "Running: autoreconf -i && ./configure $@"
autoreconf -i -Wno-portability && ./configure "$@"
