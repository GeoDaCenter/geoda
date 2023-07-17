#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 

# Build GeoDa
cp ../../GeoDamake.$OS.opt ../../GeoDamake.opt
make -j$(nproc)
make app

# Create deb#
./create_deb.sh $OS $VER