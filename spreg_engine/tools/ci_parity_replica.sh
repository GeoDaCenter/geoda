#!/usr/bin/env bash
#
# The parity job's steps, exactly as .github/workflows/spreg_engine.yml runs
# them, for running locally in an ubuntu:22.04 container:
#
#     docker run --rm -v "$PWD":/src -v /tmp/ci-parity-replica.sh:/replica.sh \
#         ubuntu:22.04 bash /replica.sh
#
# It exists because the workflow cannot be dispatched until it is on the
# default branch, and the Linux half of it - the apt packages, the CLAPACK
# build, the boost include, the virtual display - has never been exercised.
# Everything here mirrors the workflow; when the two drift apart this file is
# the one that is wrong.
#
# Written to be run where the parity job cannot be: on a machine with a
# container runtime and a slow link to Docker Hub it may be quicker to run the
# steps by hand, which is how this was checked - the CLAPACK recipe and the
# comparison both on macOS, against the published archive.

set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
    libwxgtk3.0-gtk3-dev libcurl4-openssl-dev libboost-dev xvfb \
    unzip curl ca-certificates build-essential >/dev/null

echo "== CLAPACK, as the application links it =="
curl -sL -o /tmp/clapack.tgz \
    https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
mkdir -p /tmp/clapack && tar xzf /tmp/clapack.tgz -C /tmp/clapack
cd /tmp/clapack/CLAPACK-3.2.1
cp make.inc.example make.inc
sed -i 's|^CFLAGS    = -O3 -I$(TOPDIR)/INCLUDE|& -Wno-implicit-function-declaration|' make.inc
make -j"$(nproc)" f2clib >/dev/null
cp F2CLIBS/libf2c.a .
make -j"$(nproc)" blaslib >/dev/null
make -j"$(nproc)" -C INSTALL >/dev/null
make -j"$(nproc)" -C SRC >/dev/null
mv -f blas_LINUX.a blas.a
mv -f lapack_LINUX.a lapack.a
ls -l lapack.a blas.a libf2c.a

echo "== json_spirit =="
curl -sL -o /tmp/json_spirit.zip \
    https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
unzip -q /tmp/json_spirit.zip -d /tmp/json_spirit
ls /tmp/json_spirit/json_spirit_v4.08/json_spirit/*.cpp

echo "== the published engine, as GeoDa downloads it =="
MANIFEST_SHA=$(python3 - <<'PY'
import json
m = json.load(open('/src/spreg_engine/manifest/engines.json'))
print(m['artifacts']['linux-x86_64']['sha256'])
PY
)
curl -sL -o /tmp/engine.zip \
    https://github.com/GeoDaCenter/software/releases/download/spreg-engine-v1/geoda-spreg-1.9.1-py313-linux-x86_64.zip
echo "$MANIFEST_SHA  /tmp/engine.zip" | sha256sum -c -
mkdir -p /tmp/engine && unzip -q /tmp/engine.zip -d /tmp/engine
cat /tmp/engine/engine.json

echo "== the two engines, side by side =="
cd /src
export GEODA_CLAPACK_PATH=/tmp/clapack/CLAPACK-3.2.1
export GEODA_JSON_SPIRIT_SRC=/tmp/json_spirit/json_spirit_v4.08
if command -v xvfb-run >/dev/null; then
    xvfb-run -a spreg_engine/tools/run_parity_test.sh /tmp/engine
else
    spreg_engine/tools/run_parity_test.sh /tmp/engine
fi
