#!/usr/bin/env bash
#
# MATSIMU single entry point: dependencies, compile, run.
# Default (no options): opens the desktop GUI and runs the main engine (Qt 6 if available).
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
  echo ""
  echo "Default: builds and runs the desktop GUI with the main simulation + physics engine."
  echo "If Qt is missing, the script will try to auto-install Qt 6 packages on Debian/Ubuntu."
  echo ""
  echo "Options:"
  echo "  --clean         Remove build directory and exit"
  echo "  --debug         Build with debug symbols (default: release)"
  echo "  --example NAME  Run the specified example (lattice, heat)"
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
QT_DETECT_REASON=""
QT_DEB_PACKAGES=(qt6-base-dev qt6-base-dev-tools qt6-tools-dev-tools libqt6opengl6-dev libgl1-mesa-dev)

find_qt_moc() {
  local cand=""
  cand=$(command -v moc || true)
  [[ -n "$cand" ]] && { echo "$cand"; return 0; }
  cand=$(command -v moc-qt6 || true)
  [[ -n "$cand" ]] && { echo "$cand"; return 0; }

  if command -v qtpaths6 &>/dev/null; then
    local libexec_dir
    libexec_dir=$(qtpaths6 --query QT_HOST_LIBEXECS 2>/dev/null || true)
    if [[ -n "$libexec_dir" && -x "$libexec_dir/moc" ]]; then
      echo "$libexec_dir/moc"
      return 0
    fi
  fi

  for cand in \
    /usr/lib/qt6/libexec/moc \
    /usr/lib/x86_64-linux-gnu/qt6/bin/moc \
    /usr/lib/qt6/bin/moc; do
    if [[ -x "$cand" ]]; then
      echo "$cand"
      return 0
    fi
  done
  return 1
}

configure_qt_build() {
  QT_DETECT_REASON=""
  if ! command -v pkg-config &>/dev/null; then
    QT_DETECT_REASON="pkg-config not found."
    return 1
  fi
  if ! pkg-config --exists Qt6Widgets 2>/dev/null; then
    QT_DETECT_REASON="Qt6Widgets pkg-config module not found."
    return 1
  fi
  if ! pkg-config --exists Qt6OpenGLWidgets 2>/dev/null; then
    QT_DETECT_REASON="Qt6OpenGLWidgets pkg-config module not found (need libqt6opengl6-dev)."
    return 1
  fi
  local qt_ver
  qt_ver=$(pkg-config --modversion Qt6Core 2>/dev/null || true)
  if [[ ! "$qt_ver" =~ ^6\.([0-9]+)\. ]] || (( BASH_REMATCH[1] < QT_MIN_MINOR )); then
    QT_DETECT_REASON="Qt version is '${qt_ver:-unknown}', require >= 6.${QT_MIN_MINOR}."
    return 1
  fi
  local moc_bin
  moc_bin=$(find_qt_moc || true)
  if [[ -z "$moc_bin" ]]; then
    QT_DETECT_REASON="Qt moc tool not found (checked PATH and common Qt6 install locations)."
    return 1
  fi

  USE_QT="1"
  mkdir -p "$BUILD_DIR"
  local qt_cflags
  qt_cflags=$(pkg-config --cflags Qt6Widgets Qt6OpenGLWidgets)
  QT_LIBS=$(pkg-config --libs Qt6Widgets Qt6OpenGLWidgets)
  # Some environments require explicit system OpenGL linkage in addition to Qt modules.
  if pkg-config --exists gl 2>/dev/null; then
    QT_LIBS+=" $(pkg-config --libs gl)"
  else
    QT_LIBS+=" -lOpenGL -lGL"
  fi
  CXXFLAGS+=" -DMATSIMU_USE_QT ${qt_cflags}"

  # Automate MOC for headers in matsimu/ui/
  for hpp in "${INCLUDE_UI}"/*.hpp; do
    h=$(basename "$hpp" .hpp)
    MOC_OUT="${BUILD_DIR}/moc_${h}.cpp"
    "$moc_bin" "$hpp" -o "$MOC_OUT"
    SOURCES+=("$MOC_OUT")
  done

  # Add UI implementation files
  SOURCES+=($(find "${SRC_DIR}/ui" -name "*.cpp"))
  echo "Building with Qt ${qt_ver} GUI."
  return 0
}

try_auto_install_qt() {
  if ! command -v apt-get &>/dev/null; then
    echo "Qt auto-install skipped: apt-get not available on this system."
    return 1
  fi

  echo "Qt 6 not found. Attempting automatic install: ${QT_DEB_PACKAGES[*]}"
  local -a apt_cmd=(apt-get)
  if [[ "${EUID}" -ne 0 ]]; then
    if ! command -v sudo &>/dev/null; then
      echo "Qt auto-install failed: sudo is not available."
      return 1
    fi
    if sudo -n true 2>/dev/null; then
      apt_cmd=(sudo apt-get)
    elif [[ -t 0 ]]; then
      echo "Requesting sudo privileges to install Qt packages..."
      apt_cmd=(sudo apt-get)
    else
      echo "Qt auto-install failed: non-interactive shell without passwordless sudo."
      return 1
    fi
  fi

  if "${apt_cmd[@]}" update && "${apt_cmd[@]}" install -y --no-install-recommends "${QT_DEB_PACKAGES[@]}"; then
    return 0
  fi

  # Fallback: update/install against Ubuntu source file only.
  # This bypasses failing third-party repos in /etc/apt/sources.list.d.
  local ubuntu_sources=""
  local candidate
  for candidate in \
    /etc/apt/sources.list \
    /etc/apt/sources.list.d/ubuntu.sources \
    /etc/apt/sources.list.d/official-package-repositories.list \
    /etc/apt/sources.list.d/*ubuntu*.list \
    /etc/apt/sources.list.d/*ubuntu*.sources; do
    if [[ -f "$candidate" ]]; then
      ubuntu_sources="$candidate"
      break
    fi
  done

  if [[ -z "$ubuntu_sources" ]]; then
    echo "Qt auto-install fallback failed: no Ubuntu apt source file found."
    return 1
  fi

  echo "Standard apt update failed; retrying with Ubuntu sources only: $ubuntu_sources"
  if "${apt_cmd[@]}" -o Dir::Etc::sourcelist="$ubuntu_sources" -o Dir::Etc::sourceparts="-" update \
     && "${apt_cmd[@]}" -o Dir::Etc::sourcelist="$ubuntu_sources" -o Dir::Etc::sourceparts="-" install -y --no-install-recommends "${QT_DEB_PACKAGES[@]}"; then
    return 0
  fi

  return 1
}

if ! configure_qt_build; then
  if [[ "${MATSIMU_AUTO_INSTALL_QT:-1}" == "1" ]]; then
    if try_auto_install_qt; then
      # Refresh command lookup cache after package install.
      hash -r
      configure_qt_build || true
    fi
  fi
fi

if [[ -z "$USE_QT" ]]; then
  echo "Qt 6 (>= 6.${QT_MIN_MINOR}) not found or install failed: building CLI only."
  [[ -n "$QT_DETECT_REASON" ]] && echo "  Detection detail: $QT_DETECT_REASON"
  echo "  Set MATSIMU_AUTO_INSTALL_QT=0 to skip auto-install attempts."
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

# Port selection: start from MATSIMU_PORT or default, auto-increment on conflict
MATSIMU_DEFAULT_PORT="${MATSIMU_PORT:-8765}"
find_available_port() {
  local p="${1:-$MATSIMU_DEFAULT_PORT}"
  local max_tries=500
  while (( max_tries-- > 0 )); do
    if ! ss -tuln 2>/dev/null | grep -qE ":${p}([^0-9]|\$)"; then
      echo "$p"
      return 0
    fi
    (( p++ ))
  done
  echo "" >&2
  return 1
}
PORT=$(find_available_port "$MATSIMU_DEFAULT_PORT")
if [[ -z "$PORT" ]]; then
  echo "Error: could not find an available port (tried from $MATSIMU_DEFAULT_PORT)." >&2
  exit 1
fi
[[ "$PORT" != "$MATSIMU_DEFAULT_PORT" ]] && echo "Port $MATSIMU_DEFAULT_PORT in use, using $PORT instead."
export MATSIMU_PORT="$PORT"

# Run logic: default = desktop GUI + auto-run simulation/physics engine
#            (--example runs CLI examples instead)
RUN_CMD=("$BINARY")
[[ -n "$RUN_EXAMPLE" ]] && RUN_CMD+=("--example" "$RUN_EXAMPLE")
[[ -z "$RUN_EXAMPLE" ]] && RUN_CMD+=("--autorun")

echo "Running: ${RUN_CMD[*]} $* (MATSIMU_PORT=$MATSIMU_PORT)"
exec "${RUN_CMD[@]}" "$@"
