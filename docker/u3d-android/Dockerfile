#
# Copyright (c) 2018-2022 Yao Wei Tjong. All rights reserved.
# Copyright (c) 2022-2024 the U3D project.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

FROM ubuntu:22.04

# 'base'

ARG cmake_url=https://cmake.org/files/v3.22/cmake-3.22.6-linux-x86_64.tar.gz
ARG lang=en_US.UTF-8

ENV USE_CCACHE=1 \
    CCACHE_SLOPPINESS=pch_defines,time_macros \
    CCACHE_COMPRESS=1 \
    HOST_UID=1000 \
    HOST_GID=1000 \
    DEBIAN_FRONTEND=noninteractive

RUN apt update && apt upgrade -y && \
    apt install -y --no-install-recommends \
    build-essential ccache git g++-multilib ninja-build doxygen graphviz \
    locales curl openssh-client rake rsync sudo tzdata vim wget unzip && \
    wget -qO- ${cmake_url} | tar --strip-components=1 -xzC /usr/local && \
    locale-gen ${lang} && update-locale LANG=${lang}

# 'android'

ARG tool_url=https://dl.google.com/android/repository/commandlinetools-linux-7583922_latest.zip
ARG tool_version=30.0.2
ARG ndk_version=21.4.7075529
ARG platform=30

ENV PLATFORM=android \
    ANDROID_HOME=/android-sdk \
    ANDROID_CCACHE=/usr/bin/ccache

RUN apt install -y --no-install-recommends openjdk-11-jdk-headless && \
    mkdir -p ${ANDROID_HOME}/cmdline-tools && cd ${ANDROID_HOME}/cmdline-tools && \
    wget -qO- ${tool_url} -O cmdline-tools.zip && \
    unzip cmdline-tools.zip && rm cmdline-tools.zip && \
    mv cmdline-tools latest && cd latest/bin && \
    yes | ./sdkmanager --install "build-tools;${tool_version}" "ndk;${ndk_version}" "platforms;android-${platform}" && \
    yes | ./sdkmanager --uninstall "emulator" && \
    yes | ./sdkmanager --licenses

RUN apt clean

VOLUME /home/U3D

COPY sysroot/ /

ENTRYPOINT ["/entrypoint.sh"]

CMD ["/usr/bin/rake"]
