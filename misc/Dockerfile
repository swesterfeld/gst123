FROM gcc:latest

RUN apt-get update && apt-get install -y \
    build-essential clang libgtk-3-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

ADD . /gst123
WORKDIR /gst123

RUN misc/build.sh
