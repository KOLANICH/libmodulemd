#!/bin/bash

#Exit on failures
set -e

set -x

JOB_NAME=${TRAVIS_JOB_NAME:-Arch Linux}

os_name='Arch Linux'
release=base


pushd /builddir/

# Build the code under GCC and run standard tests
meson --buildtype=debug \
      -Dtest_dirty_git=false \
      -Ddeveloper_build=false \
      -Dpython_name=python3 \
      -Drpmio=disabled \
      travis

ninja -C travis test

popd #builddir
