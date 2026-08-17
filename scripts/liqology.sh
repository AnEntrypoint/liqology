#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

usage() {
    cat <<'EOF'
liqology - cargo-equivalent wrapper over CMakePresets + vcpkg

Usage:
  liqology.sh new <name>          Scaffold a new consumer project from this template
  liqology.sh add <package>       Add a vcpkg dependency to vcpkg.json (cargo add equivalent)
  liqology.sh build [preset]      Configure + build (default preset: release)
  liqology.sh run [preset] [args] Build then run the hello example
  liqology.sh test-sanitize       Build + run under asan-ubsan preset
  liqology.sh tidy                Run clang-tidy over include/
EOF
}

require_vcpkg_root() {
    if [ -z "${VCPKG_ROOT:-}" ]; then
        echo "error: VCPKG_ROOT is not set." >&2
        echo "  liqology needs VCPKG_ROOT pointing at a vcpkg checkout to resolve dependencies." >&2
        echo "  Fix: git clone https://github.com/microsoft/vcpkg && export VCPKG_ROOT=\"\$PWD/vcpkg\" && \"\$VCPKG_ROOT/bootstrap-vcpkg.sh\"" >&2
        exit 1
    fi
}

cmd_new() {
    local name="${1:?usage: liqology.sh new <name>}"
    mkdir -p "$name"
    cp -r "$REPO_ROOT"/{CMakeLists.txt,CMakePresets.json,vcpkg.json,vcpkg-configuration.json,.clang-tidy,cmake,vcpkg-triplets,include} "$name"/
    mkdir -p "$name/src"
    echo "Scaffolded $name from liqology template."
}

cmd_add() {
    local package="${1:?usage: liqology.sh add <package>}"
    node -e '
        const fs = require("fs");
        const path = process.argv[1];
        const pkg = process.argv[2];
        const manifest = JSON.parse(fs.readFileSync(path, "utf8"));
        if (!Array.isArray(manifest.dependencies)) manifest.dependencies = [];
        const already = manifest.dependencies.some(d => (typeof d === "string" ? d : d.name) === pkg);
        if (already) {
            console.log(`${pkg} is already a dependency, no change made.`);
        } else {
            manifest.dependencies.push(pkg);
            fs.writeFileSync(path, JSON.stringify(manifest, null, 2) + "\n");
            console.log(`Added ${pkg} to vcpkg.json.`);
        }
    ' "$REPO_ROOT/vcpkg.json" "$package"
}

cmd_build() {
    require_vcpkg_root
    local preset="${1:-release}"
    if [ "$preset" = "wasip1-release" ] && [ -z "${WASI_SDK_PREFIX:-}" ]; then
        echo "error: WASI_SDK_PREFIX is not set (required for the wasip1-release preset)." >&2
        echo "  Fix: download wasi-sdk from https://github.com/WebAssembly/wasi-sdk/releases and" >&2
        echo "       export WASI_SDK_PREFIX=/path/to/wasi-sdk" >&2
        exit 1
    fi
    cmake --preset "$preset" -S "$REPO_ROOT" -B "$REPO_ROOT/build/$preset"
    cmake --build "$REPO_ROOT/build/$preset"
}

cmd_run() {
    local preset="${1:-release}"
    shift || true
    cmd_build "$preset"
    "$REPO_ROOT/build/$preset/examples/hello/liqology_hello" "$@"
}

cmd_test_sanitize() {
    cmd_build asan-ubsan
    "$REPO_ROOT/build/asan-ubsan/examples/hello/liqology_hello"
}

cmd_tidy() {
    cmd_build debug
    clang-tidy -p "$REPO_ROOT/build/debug" "$REPO_ROOT"/include/liqology/*.hpp
}

case "${1:-}" in
    new) shift; cmd_new "$@" ;;
    add) shift; cmd_add "$@" ;;
    build) shift; cmd_build "$@" ;;
    run) shift; cmd_run "$@" ;;
    test-sanitize) cmd_test_sanitize ;;
    tidy) cmd_tidy ;;
    *) usage; exit 1 ;;
esac
