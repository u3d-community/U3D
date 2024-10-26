#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 Yao Wei Tjong. All rights reserved.
# Copyright (c) 2022-2024 the U3D project.
#
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

# Check if the host project root is correctly mounted before proceeding further
if [[ ! $PROJECT_DIR || -z $(ls -1 $PROJECT_DIR 2>/dev/null) ]]; then
    echo -e "Error: PROJECT_DIR env-var must be set to project root directory from host.
       See 'script/dockerized.sh' in the Urho3D project as use case sample."
    exit 1
fi

# Enable history-search in bash completion
sed -i '/^#.*history-search/s/^# //' /etc/inputrc

# Delay updating the symlinks to runtime after all compiler toolchains have been installed
/usr/sbin/update-ccache-symlinks

# Ensure ccache is being found first
PATH=/usr/lib/ccache:$ANDROID_HOME/cmdline-tools/latest/bin:$PATH

# Use the built-in locale from the docker image
set -a && . /etc/default/locale && set +a

if [[ "$container" == "podman" ]]; then
    # Change home dir for this session
    HOME=/home/U3D
    # Set Gradle user home to a dir mounted from pod's volume so Gradle caches are persisted
    export GRADLE_USER_HOME=~/.gradle
    # Set Maven local repository to a mounted dir for similar reason as above
    mkdir -p /root/.m2
    echo '<settings><localRepository>/home/U3D/.m2/repository</localRepository></settings>' >/root/.m2/settings.xml
    
    echo "EXECUTE podman project:$PROJECT_DIR $@"
    # Execute the command chain (relative to project root) as is since we run Podman in rootless mode
    cd $PROJECT_DIR && exec "$@"
else
    # Delay group and user creation to runtime to match the host GID and host UID
    groupadd -g $HOST_GID U3D && useradd -u $HOST_UID -g $HOST_GID -s /bin/bash U3D

    # Allow 'U3D' user to write into mounted docker volumes
    chmod o+w /home/U3D

    # With great power comes great responsibility
    echo "U3D ALL=(ALL) NOPASSWD: ALL" >/etc/sudoers.d/super_u3d

    echo "EXECUTE project:$PROJECT_DIR $@"
    # Execute the command chain (relative to project root) as urho3d
    cd $PROJECT_DIR && runuser -u U3D -- "$@"
fi


