#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

docker build -f "misc/Dockerfile" -t gst123-dbuild .
docker build -f "misc/Dockerfile-arch" -t gst123-dbuild-arch .
