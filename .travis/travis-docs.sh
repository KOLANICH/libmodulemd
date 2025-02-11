#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $SCRIPT_DIR

set -e
set -x

JOB_NAME=${TRAVIS_JOB_NAME:-Fedora rawhide}

# Always generate the docs on Fedora Rawhide
os_name=Fedora
release=rawhide

# Create an archive of the current checkout
TARBALL_PATH=`mktemp -p $SCRIPT_DIR tarball-XXXXXX.tar.bz2`
TARBALL=`basename $TARBALL_PATH`

pushd $SCRIPT_DIR/..
git ls-files |xargs tar cfj $TARBALL_PATH .git
popd

repository="registry.fedoraproject.org"
os="fedora"

sed -e "s/@IMAGE@/$repository\/$os:$release/" \
    $SCRIPT_DIR/fedora/Dockerfile.deps.tmpl > $SCRIPT_DIR/fedora/Dockerfile.deps.$release

sudo docker build \
    -f $SCRIPT_DIR/fedora/Dockerfile.deps.$release \
    -t fedora-modularity/libmodulemd-deps-$release .

sudo docker build \
    -f $SCRIPT_DIR/docs/Dockerfile \
    -t fedora-modularity/libmodulemd-docs \
    --build-arg TARBALL=$TARBALL .

rm -f $TARBALL_PATH $SCRIPT_DIR/fedora/Dockerfile.deps.$release $SCRIPT_DIR/fedora/Dockerfile-$release

# Override the standard tasks with the doc-generation
docker run \
    -e TRAVIS=$TRAVIS \
    -e TRAVIS_JOB_NAME="$TRAVIS_JOB_NAME" \
    -e TRAVIS_COMMIT="$TRAVIS_COMMIT" \
    -e DOC_TOKEN="$DOC_TOKEN" \
    --rm fedora-modularity/libmodulemd-docs

popd
exit 0
