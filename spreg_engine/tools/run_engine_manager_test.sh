#!/usr/bin/env bash
#
# Build and run the headless test of Regression/SpregEngine.cpp.
#
#     spreg_engine/tools/run_engine_manager_test.sh [path/to/engine.zip]
#
# It compiles just SpregEngine.cpp and the test harness, against wxWidgets,
# libcurl and json_spirit - not the whole application - and exercises the
# verify / unpack / install / discover / upgrade / remove paths against a real
# engine archive in a scratch directory.
#
# Environment: WX_CONFIG, CURL_CONFIG, GEODA_CMAKE_BUILD (to find json_spirit).

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WX_CONFIG=${WX_CONFIG:-wx-config}
CURL_CONFIG=${CURL_CONFIG:-curl-config}

ARCHIVE=${1:-}
if [ -z "$ARCHIVE" ]; then
	ARCHIVE=$(ls -1 "$ROOT"/spreg_engine/dist/*.zip 2>/dev/null | head -1 || true)
fi
if [ -z "$ARCHIVE" ] || [ ! -f "$ARCHIVE" ]; then
	echo "usage: $0 <engine-archive.zip>" >&2
	echo "       (build one first: spreg_engine/tools/build_engine.py --outdir dist)" >&2
	exit 2
fi

# json_spirit comes from the CMake build if there is one, otherwise from source
JSON_SPIRIT_LIB=""
JSON_SPIRIT_SRC=""
for candidate in "${GEODA_CMAKE_BUILD:-}" "$HOME/github/geoda/build" "$ROOT/build"; do
	[ -z "$candidate" ] && continue
	if [ -f "$candidate/libjson_spirit.a" ]; then
		JSON_SPIRIT_LIB="$candidate/libjson_spirit.a"
		JSON_SPIRIT_SRC="$candidate/_deps/json_spirit-src"
		break
	fi
done

WORK=$(mktemp -d "${TMPDIR:-/tmp}/geoda-engine-test-XXXXXX")
trap 'rm -rf "$WORK"' EXIT
BIN="$WORK/test_engine_manager"

echo "archive : $ARCHIVE"
echo "work    : $WORK"

JSON_ARGS=()
if [ -n "$JSON_SPIRIT_LIB" ]; then
	echo "json_spirit: $JSON_SPIRIT_LIB"
	# json_spirit wants boost headers, which live next to wx on this platform
	JSON_ARGS+=("-I$JSON_SPIRIT_SRC" "-I${BOOST_INCLUDE:-/opt/homebrew/include}"
	            "$JSON_SPIRIT_LIB")
else
	echo "json_spirit: not found, please set GEODA_CMAKE_BUILD" >&2
	exit 2
fi

# shellcheck disable=SC2046
"${CXX:-c++}" -std=gnu++14 -O1 -g -o "$BIN" \
	-I"$ROOT" \
	$($WX_CONFIG --cxxflags) \
	"$ROOT/Regression/SpregEngine.cpp" \
	"$ROOT/Regression/SpregJob.cpp" \
	"$ROOT/spreg_engine/tools/test_engine_manager.cpp" \
	$($WX_CONFIG --libs std) \
	$($CURL_CONFIG --libs) \
	"${JSON_ARGS[@]}"

echo
"$BIN" "$ARCHIVE" "$WORK"
