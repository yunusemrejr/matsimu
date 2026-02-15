# MATSIMU Architecture

## Entry point

- **Single entry**: `run.sh` handles dependencies, compilation, execution, and tests. No separate build scripts.
- **Tests**: `./run.sh --test` builds and runs the test binary; default build/run is unchanged.

## Directory layout

- **include/matsimu/** — Public API by layer:
  - **core/** — Types (`Real`, `Index`), unit system constants.
  - **alloc/** — Resource-aware allocators (bounded, fail-fast).
  - **lattice/** — Lattice basis, volume, min-image (3D/material).
  - **sim/** — Simulation orchestration, `ISimModel` interface, params, time stepping; model-specific kernels (e.g. heat diffusion).
  - **io/** — Config load (`ConfigResult`), parser/validator; conversions at I/O boundary only.
  - **ui/** — Main window (timer-driven non-blocking run), tabs (Simulation, Lattice, 3D View); Qt 6.2+.
- **src/** — Implementation (.cpp); one-to-one or shared by module.
- **tests/** — C++ unit and integration tests (parameter validation, stability, lattice, config, deterministic stepping).
- **examples/** — Runnable examples; invokable via `./run.sh --example <name>` or documented preset flows.

## Simulation and models

- **Simulation** orchestrates stepping and termination; it delegates the math to model-specific code via **ISimModel** (e.g. molecular dynamics, heat diffusion).
- **Heat diffusion**: 1D explicit scheme with invariants α > 0, dx > 0, dt ≤ stability_limit (dx²/(2α)); SI throughout; conversions at I/O only.

## Config contract

- **Empty path**: `load_config("")` returns default params (no error).
- **Non-empty path**: read file; on success return parsed params (SI); on file-not-found or parse error return `ok = false` and a non-empty error message (no silent defaults).

## Dependency rules

- Dependencies point **inward**: app → sim/io/lattice → core/alloc. No circular deps.
- **core** and **alloc** do not depend on lattice, sim, or io.
- **io** may depend on sim (e.g. load `SimulationParams`); conversions (units, format) at I/O boundary only.

## Invariants

- Headers declare public API; implementation in .cpp.
- One responsibility per header (or small coherent set).
- Shared logic in one place (DRY).
- Resource limits enforced in code (allocators throw on exhaustion).
- Unit system: SI in core; document and convert at I/O only.
- Stability limits (e.g. diffusion) enforced in model params validation.

## Adding code

1. New module: add `include/matsimu/<layer>/<name>.hpp` and optionally `src/<layer>/<name>.cpp`. Sources are discovered by `run.sh` (non-UI under `src/`; UI when Qt present). Add tests under `tests/` and run with `./run.sh --test`.
2. New example: add to `examples/` and wire `--example <name>` in main if desired.
3. Preserve no cycles and explicit dependency direction.
