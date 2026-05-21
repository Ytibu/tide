#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-${ROOT_DIR}/install}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_TESTS="${BUILD_TESTS:-OFF}"
JOBS="${JOBS:-$(nproc)}"
GENERATOR="${GENERATOR:-Unix Makefiles}"

usage() {
	cat <<'EOF'
Usage: ./generate.sh [options]

Options:
  --build-dir DIR       Build directory (default: ./build)
  --prefix DIR          Install prefix (default: ./install)
  --build-type TYPE     CMake build type (default: Release)
  --with-tests          Build tests under tests/
  --jobs N              Parallel build jobs (default: nproc)
  --clean               Remove the build directory before configuring
  -h, --help            Show this help message
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir)
			BUILD_DIR="$2"
			shift 2
			;;
		--prefix)
			INSTALL_PREFIX="$2"
			shift 2
			;;
		--build-type)
			BUILD_TYPE="$2"
			shift 2
			;;
		--with-tests)
			BUILD_TESTS=ON
			shift
			;;
		--jobs)
			JOBS="$2"
			shift 2
			;;
		--clean)
			rm -rf "$BUILD_DIR"
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
	-DTIDE_BUILD_TESTS="$BUILD_TESTS"

make -C "$BUILD_DIR" -j"$JOBS"
make -C "$BUILD_DIR" install

echo "Build complete."
echo "Install prefix: $INSTALL_PREFIX"
