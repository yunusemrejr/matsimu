#!/usr/bin/env bash
#
# MATSIMU single entry point: dependencies, compile, run.
# Usage: ./run.sh [--clean] [--debug] [--example NAME] [--test] [--] [args...]
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BINARY="${BUILD_DIR}/matsimu"
TEST_BINARY="${BUILD_DIR}/matsimu_test"
INCLUDE_DIR="${SCRIPT_DIR}/include"
SRC_DIR="${SCRIPT_DIR}/src"
TESTS_DIR="${SCRIPT_DIR}/tests"
INCLUDE_UI="${INCLUDE_DIR}/matsimu/ui"

show_usage() {
  echo "Usage: $0 [OPTIONS] [-- [BINARY_ARGS]]"
  echo "Options:"
  echo "  --clean         Remove build directory and exit"
  echo "  --debug         Build with debug symbols (default: release)"
  echo "  --example NAME  Run the specified example"
  echo "  --test          Build and run C++ tests (unit + integration), then exit"
  echo "  -h, --help      Show this help message"
  exit 0
}

# Defaults
BUILD_TYPE="release"
RUN_EXAMPLE=""
RUN_TEST=false
CLEAN=false

# CLI parsing
while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)    CLEAN=true; shift ;;
    --debug)   BUILD_TYPE="debug"; shift ;;
    --example) RUN_EXAMPLE="${2:-}"; shift 2 ;;
    --test)    RUN_TEST=true; shift ;;
    -h|--help) show_usage ;;
    --)        shift; break ;;
    *)         break ;;
  esac
done

if [[ "$CLEAN" == true ]]; then
  rm -rf "$BUILD_DIR"
  echo "Cleaned $BUILD_DIR"
  exit 0
fi

# Compiler and Flags
CXX="${CXX:-g++}"
if ! command -v "$CXX" &>/dev/null; then
  echo "Error: C++ compiler '$CXX' not found. Install with: sudo apt install build-essential" >&2
  exit 1
fi

CXXFLAGS="-Wall -Wextra"
if [[ "$BUILD_TYPE" == "debug" ]]; then
  CXXFLAGS+=" -g -O0 -DDEBUG"
else
  CXXFLAGS+=" -O2 -DNDEBUG"
fi

# Sources: Automate discovery
SOURCES=($(find "${SRC_DIR}" -maxdepth 2 -name "*.cpp" ! -path "*/ui/*"))

# Qt 6 UI Detection (requires >= 6.2; all APIs used are available since 6.2)
USE_QT=""
QT_LIBS=""
QT_MIN_MINOR=2
if pkg-config --exists Qt6Widgets Qt6OpenGLWidgets 2>/dev/null; then
  QT_VER=$(pkg-config --modversion Qt6Core 2>/dev/null)
  if [[ "$QT_VER" =~ ^6\.([0-9]+)\. ]] && (( BASH_REMATCH[1] >= QT_MIN_MINOR )); then
    MOC=$(command -v moc || command -v moc-qt6 || echo "")
    if [[ -n "$MOC" ]]; then
      USE_QT="1"
      mkdir -p "$BUILD_DIR"
      QT_CFLAGS=$(pkg-config --cflags Qt6Widgets Qt6OpenGLWidgets)
      QT_LIBS=$(pkg-config --libs Qt6Widgets Qt6OpenGLWidgets)
      CXXFLAGS+=" -DMATSIMU_USE_QT ${QT_CFLAGS}"
      
      # Automate MOC for headers in matsimu/ui/
      for hpp in "${INCLUDE_UI}"/*.hpp; do
        h=$(basename "$hpp" .hpp)
        MOC_OUT="${BUILD_DIR}/moc_${h}.cpp"
        "$MOC" "$hpp" -o "$MOC_OUT"
        SOURCES+=("$MOC_OUT")
      done
      
      # Add UI implementation files
      SOURCES+=($(find "${SRC_DIR}/ui" -name "*.cpp"))
      echo "Building with Qt ${QT_VER} GUI."
    fi
  fi
fi

if [[ -z "$USE_QT" ]]; then
  echo "Qt 6 (>= 6.${QT_MIN_MINOR}) not found or moc missing: building CLI only."
  echo "  Install with: sudo apt install qt6-base-dev qt6-base-dev-tools"
fi

mkdir -p "$BUILD_DIR"

if [[ "$RUN_TEST" == true ]]; then
  # Test binary: all lib sources (no main, no UI) + tests
  LIB_SOURCES=($(find "${SRC_DIR}" -maxdepth 2 -name "*.cpp" ! -path "*/ui/*" ! -name "main.cpp"))
  TEST_SOURCES=("${LIB_SOURCES[@]}")
  if [[ -f "${TESTS_DIR}/test_main.cpp" ]]; then
    TEST_SOURCES+=("${TESTS_DIR}/test_main.cpp")
  fi
  echo "Compiling tests (${BUILD_TYPE})..."
  if ! "$CXX" $CXXFLAGS -I"$INCLUDE_DIR" -std=c++17 "${TEST_SOURCES[@]}" -o "$TEST_BINARY"; then
    echo "Test build failed." >&2
    exit 1
  fi
  echo "Running tests..."
  exec "$TEST_BINARY"
fi

echo "Compiling (${BUILD_TYPE})..."
COMPILE_CMD=("$CXX" $CXXFLAGS -I"$INCLUDE_DIR" -std=c++17 "${SOURCES[@]}" -o "$BINARY")
[[ -n "$USE_QT" ]] && COMPILE_CMD+=("-fPIC")

if ! "${COMPILE_CMD[@]}" $QT_LIBS; then
  echo "Compilation failed." >&2
  exit 1
fi

# Run logic
RUN_CMD=("$BINARY")
[[ -n "$RUN_EXAMPLE" ]] && RUN_CMD+=("--example" "$RUN_EXAMPLE")

echo "Running: ${RUN_CMD[*]} $*"
exec "${RUN_CMD[@]}" "$@"
