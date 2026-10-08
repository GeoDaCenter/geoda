#!/usr/bin/env bash
#
# Build and run the parity test: GeoDa's own engine against spreg, on the same
# grid, the same weights and the same three models they have in common.
#
#     spreg_engine/tools/run_parity_test.sh [engine-dir]
#
# The engine directory defaults to the one GeoDa installs, and can be any
# unpacked engine archive.  It compiles the two engines and the comparison - not
# the whole application - so it runs in seconds.
#
# Environment: GEODA_CMAKE_BUILD (json_spirit), GEODA_CLAPACK_PATH (lapack, blas,
# f2c), WX_CONFIG, CURL_CONFIG.

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WX_CONFIG=${WX_CONFIG:-wx-config}
CURL_CONFIG=${CURL_CONFIG:-curl-config}

ENGINE=${1:-}
if [ -z "$ENGINE" ]; then
	case "$(uname -s)" in
		Darwin) ENGINE="$HOME/Library/Application Support/GeoDa/engines" ;;
		*)      ENGINE="$HOME/.geoda/engines" ;;
	esac
	ENGINE=$(ls -d "$ENGINE"/spreg-* 2>/dev/null | head -1 || true)
fi
if [ -z "$ENGINE" ] || [ ! -d "$ENGINE" ]; then
	echo "usage: $0 <engine-dir>" >&2
	echo "       (install one from GeoDa, or unpack an archive from" >&2
	echo "        https://github.com/GeoDaCenter/software/releases/tag/spreg-engine-v1)" >&2
	exit 2
fi

# json_spirit: the static library from a GeoDa build if there is one, and
# otherwise its sources, which are four files and build in a second
JSON_SPIRIT_LIB=""
JSON_SPIRIT_SRC="${GEODA_JSON_SPIRIT_SRC:-}"
for candidate in "${GEODA_CMAKE_BUILD:-}" "$HOME/github/geoda/build" "$ROOT/build"; do
	[ -z "$candidate" ] && continue
	if [ -f "$candidate/libjson_spirit.a" ]; then
		JSON_SPIRIT_LIB="$candidate/libjson_spirit.a"
		JSON_SPIRIT_SRC="$candidate/_deps/json_spirit-src"
		break
	fi
done
if [ -z "$JSON_SPIRIT_SRC" ]; then
	for candidate in "$ROOT"/BuildTools/*/dep/json_spirit /tmp/json_spirit; do
		[ -d "$candidate" ] && JSON_SPIRIT_SRC="$candidate" && break
	done
fi
JSON_SPIRIT_OBJECTS=""
if [ -z "$JSON_SPIRIT_LIB" ]; then
	if [ -z "$JSON_SPIRIT_SRC" ]; then
		echo "json_spirit not found; set GEODA_CMAKE_BUILD (a GeoDa build directory)" >&2
		echo "or GEODA_JSON_SPIRIT_SRC (json_spirit_v4.08, as the build scripts fetch)" >&2
		exit 2
	fi
	echo "json_spirit: building from $JSON_SPIRIT_SRC"
	for source in "$JSON_SPIRIT_SRC"/json_spirit/*.cpp; do
		"${CXX:-c++}" -std=gnu++14 -O1 -c "$source" -o "$WORK/$(basename "$source" .cpp).o" \
			-I"$JSON_SPIRIT_SRC" -I"${BOOST_INCLUDE:-/usr/include}"
		JSON_SPIRIT_OBJECTS="$JSON_SPIRIT_OBJECTS $WORK/$(basename "$source" .cpp).o"
	done
fi
JSON_SPIRIT_LINK="${JSON_SPIRIT_LIB:-$JSON_SPIRIT_OBJECTS}"

# LAPACK, as the application links it
CLAPACK=${GEODA_CLAPACK_PATH:-$ROOT/BuildTools/macosx/temp/CLAPACK-3.2.1}
if [ ! -f "$CLAPACK/lapack.a" ]; then
	echo "CLAPACK not found at $CLAPACK; set GEODA_CLAPACK_PATH" >&2
	exit 2
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/geoda-parity-XXXXXX")
trap 'rm -rf "$WORK"' EXIT

echo "engine  : $ENGINE"
echo "clapack : $CLAPACK"

# shellcheck disable=SC2046
"${CXX:-c++}" -std=gnu++14 -O1 -g -o "$WORK/test_parity" \
	-I"$ROOT" -I"$JSON_SPIRIT_SRC" -I"${BOOST_INCLUDE:-/opt/homebrew/include}" \
	$( $WX_CONFIG --cxxflags 2>/dev/null || echo "" ) \
	"$ROOT/Regression/SpregEngine.cpp" \
	"$ROOT/Regression/SpregJob.cpp" \
	"$ROOT/Regression/smile2.cpp" \
	"$ROOT/Regression/ML_im.cpp" \
	"$ROOT/Regression/mix.cpp" \
	"$ROOT/Regression/PowerLag.cpp" \
	"$ROOT/Regression/PowerSymLag.cpp" \
	"$ROOT/Regression/DenseMatrix.cpp" \
	"$ROOT/Regression/DenseVector.cpp" \
	"$ROOT/Regression/SparseMatrix.cpp" \
	"$ROOT/Regression/SparseRow.cpp" \
	"$ROOT/Regression/SparseVector.cpp" \
	"$ROOT/Regression/Weights.cpp" \
	"$ROOT/Regression/DiagnosticReport.cpp" \
	"$ROOT/ShapeOperations/GalWeight.cpp" \
	"$ROOT/ShapeOperations/GeodaWeight.cpp" \
	"$ROOT/spreg_engine/tools/test_parity.cpp" \
	$( $WX_CONFIG --libs std 2>/dev/null || echo "" ) \
	$( $CURL_CONFIG --libs ) \
	$JSON_SPIRIT_LINK \
	"$CLAPACK/lapack.a" "$CLAPACK/blas.a" "$CLAPACK/libf2c.a"

echo
"$WORK/test_parity" "$ENGINE" "$WORK" "$ROOT"
