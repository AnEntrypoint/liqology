# liqology

A liquid ontology platform for codifying LLM/agent interactions over time: every call's input/output pair is embedded, compared against prior activity via FAISS, and continuously reinforced-or-pruned by relevance — so an agent's accumulated context stays useful without growing unboundedly.

Built on a C++23 boilerplate that closes the ergonomic and distributability gap between C/C++ and Rust — without leaving the C++ ecosystem.

## Architecture

| Stage | Mechanism |
| --- | --- |
| Disambiguation | FAISS `IndexFlatIP` vector search over embeddings (`liqology::DisambiguationIndex`), returning scored candidates |
| Bucketing | Disambiguated entities are grouped into neuron-connectable `Bucket`s, each carrying its own hidden state and time-constant `tau` |
| Refinement | A `LiquidCore` runs a parallelizable Euler-discretized liquid time-constant (LTC) update per query, propagating context through bucket hidden states without cross-query coupling — the "PLAN" architecture ([arxiv 2608.03041](https://arxiv.org/abs/2608.03041)) |
| Memory | `liqology::MemoryStore` records every LLM/agent call's input/output pair, reinforces entries whose output is similar to a new one (FAISS comparison), decays the rest, and prunes anything whose estimated re-informing cost no longer justifies its retain cost — see [`docs/adr/0002-reinforce-prune-cost-balance.md`](docs/adr/0002-reinforce-prune-cost-balance.md) |
| Skill wrapper | [`skill/liqology-memory/SKILL.md`](skill/liqology-memory/SKILL.md) documents wrapping agent calls so every interaction feeds `MemoryStore::record` |

Bucket relationships live only for the duration of one query and dissolve fully on scope exit — no cross-bucket coupling survives past a query's acknowledgment. See `examples/memory-loop` for a runnable demonstration of the reinforce/prune loop.

## Gap closed (boilerplate layer)

| Rust mechanism | liqology mechanism |
| --- | --- |
| `Result<T, E>` | `liqology::Result<T, E>` over `std::expected` (fallback: `tl::expected`) |
| `Option<T>` | `std::optional<T>` used directly at API boundaries |
| Ownership/borrow checker | `gsl::not_null`, `gsl::span`, `gsl::owner`, `Box<T>`/`Rc<T>` aliases over `unique_ptr`/`shared_ptr`, enforced by `.clang-tidy`'s `cppcoreguidelines-*`/`bugprone-*` gate |
| `cargo` | `vcpkg.json` manifest mode + `CMakePresets.json`, both pinned, both driven by `scripts/liqology.sh`/`.ps1` |
| `cargo new` / `build` / `run` | `liqology.sh new` / `build` / `run` |
| `cargo add <crate>` | `liqology.sh add <package>` appends to `vcpkg.json` |
| `cargo test` + Miri-class UB detection | `liqology.sh test-sanitize` (ASan + UBSan preset), gated in CI |
| `rustup target add` + cross-compile | CMakePresets triplets for x64/arm64 across Linux/Windows/macOS, plus a `wasm32-wasip1` toolchain + vcpkg overlay triplet |
| `cargo build --release` publishing to crates.io | `.github/workflows/release.yml`: tag push cross-builds every target + wasip1, uploads as GitHub Release assets |

## Quickstart

Windows (PowerShell, no WSL/git-bash required):

```powershell
git clone https://github.com/AnEntrypoint/liqology
cd liqology
.\scripts\liqology.ps1 run
```

Linux / macOS:

```bash
git clone https://github.com/AnEntrypoint/liqology
cd liqology
./scripts/liqology.sh run
```

Scaffold a new project from this template:

```powershell
.\scripts\liqology.ps1 new my-project
cd my-project
..\liqology\scripts\liqology.ps1 run
```

Add a vcpkg dependency:

```powershell
.\scripts\liqology.ps1 add fmt
```

Run under AddressSanitizer + UndefinedBehaviorSanitizer:

```powershell
.\scripts\liqology.ps1 test-sanitize
```

Run clang-tidy (C++ Core Guidelines, treated as the borrow-checker-equivalent static gate):

```powershell
.\scripts\liqology.ps1 tidy
```

## Target matrix

Native release presets: `x64-linux-release`, `arm64-linux-release`, `x64-windows-release`, `arm64-windows-release`, `x64-macos-release`, `arm64-macos-release`.

WebAssembly: `wasip1-release` (requires `WASI_SDK_PREFIX` pointing at a wasi-sdk install), producing a `.wasm` module runnable under wasmtime.

Sanitizer presets (native, non-wasip1, non-MSVC only): `asan`, `ubsan`, `asan-ubsan`.

Every `liqology.sh`/`.ps1` build command checks `VCPKG_ROOT` (and `WASI_SDK_PREFIX` for `wasip1-release`) up front and prints a specific, actionable fix if either is missing, instead of surfacing CMake's/vcpkg's own generic toolchain error.
