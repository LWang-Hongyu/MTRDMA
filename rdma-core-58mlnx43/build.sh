#!/bin/bash
set -e

CRTDIR=$(pwd)
SRCDIR=`dirname $0`
BUILDDIR="$SRCDIR/build"

mkdir -p "$BUILDDIR"

CMAKE=cmake

cd "$BUILDDIR"

$CMAKE -DIN_PLACE=1 -DNO_PYVERBS=1 ${EXTRA_CMAKE_FLAGS:-} ..
make

echo -e " \033[36m Finish! \033[0m \033[41;37m GuoLab \033[0m"