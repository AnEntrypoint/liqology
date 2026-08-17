# liqology

Build and modify C++23 code in a liqology-based project with Rust-equivalent safety guarantees, using this repo's own tooling rather than raw compiler/build invocations.

## Invariants (non-negotiable, every emission)

- **Errors are `liqology::Result<T, E>`, never exceptions.** Include `liqology/result.hpp`. Return `liqology::Fail<E, T>(err)` / `liqology::Ok<T, E>(val)`. Never `throw`. Compile with `-fno-exceptions` to make violations a compile error.
- **No raw owning pointers, ever.** Include `liqology/ownership.hpp`. Use `liqology::Box<T>` (unique ownership), `liqology::Rc<T>` (shared ownership), `liqology::NotNull<T*>` (non-null contract at API boundaries), `liqology::Span<T>` (bounds-checked array view). Never `new`/`delete`, never a bare `T*` parameter that could be null without a documented reason.
- **Every change is verified via this repo's own scripts, never raw `cmake`/`clang++` invocations directly** (except transient standalone syntax checks during authoring, which must still be followed by the real preset build before declaring done). Use `scripts/liqology.ps1` on Windows (no WSL/git-bash dependency required), `scripts/liqology.sh` on Linux/macOS:
  - `add <package>` — append a vcpkg dependency to `vcpkg.json` (cargo-add equivalent). Idempotent.
  - `build [preset]` — configure + build a preset. Checks `VCPKG_ROOT`/`WASI_SDK_PREFIX` up front with an actionable error if missing.
  - `tidy` — clang-tidy gate (C++ Core Guidelines checks, the borrow-checker-equivalent static pass). Must return zero warnings before a change is done.
  - `test-sanitize` — ASan + UBSan build and run. Must exit 0 on the real path before a change is done.
  - `run [preset] [args]` — build then run the example.
- **Cross-target claims require the real preset, not an assumption.** `cmake --list-presets` names every supported triplet (native x64/arm64 across Linux/Windows/macOS, plus `wasip1-release`). A change claimed "portable" without exercising the relevant preset (locally if the toolchain is present, otherwise via this repo's CI matrix in `.github/workflows/ci.yml`) is an unwitnessed claim.
- **`.clang-tidy` at repo root is the sole style/safety authority.** Never add a per-file suppression without a `// NOLINT(check-name): reason` comment naming why.

## Workflow

1. Read the existing header/example shapes in `include/liqology/` and `examples/hello/main.cpp` before adding new code — match the established Result/ownership idiom rather than inventing a parallel one.
2. Write the change using `Result<T,E>` and the `liqology::` ownership aliases exclusively.
3. Run `./scripts/liqology.sh tidy` — fix every warning before proceeding.
4. Run `./scripts/liqology.sh test-sanitize` — a clean run is required; any ASan/UBSan abort means the change has a real memory-safety or UB defect to fix, not suppress.
5. If the change touches a specific target triplet's concerns (wasip1 pthread-avoidance, MSVC-specific sanitizer absence, etc.), verify against that preset directly, or push and watch the corresponding CI matrix leg.
6. Never leave a `TODO`/`FIXME`/deferred-verification marker in place of running steps 3–5.

## Common commands (verified against this repo)

```bash
cmake --list-presets                     # enumerate every declared build target
./scripts/liqology.sh build release     # native release build
./scripts/liqology.sh test-sanitize     # ASan+UBSan build and run — must exit 0
./scripts/liqology.sh tidy              # clang-tidy gate — must be warning-free
./scripts/liqology.sh run release 7     # build then run the hello example with arg 7
```
