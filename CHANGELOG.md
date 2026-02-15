# Changelog

All notable user-visible changes to MATSIMU are documented here.

## [Unreleased]

- Heat diffusion module: 1D explicit scheme with α, dx, dt and stability check (dt ≤ dx²/(2α)).
- Simulation orchestrates stepping; model-specific math in `ISimModel` (e.g. heat diffusion).
- Config: `load_config(path)` — empty path returns defaults; non-empty path returns explicit error on failure (no silent defaults). Key=value file format.
- CLI: `--config PATH`, `--example heat`, deterministic run with clear status output.
- GUI: timer-driven non-blocking simulation loop; Simulation and Lattice tabs with plain-language intros and tooltips.
- Tests: `./run.sh --test` for parameter validation, stability, lattice, config, deterministic stepping.
- Docs and examples updated; LICENSE, CONTRIBUTING.md, CHANGELOG.md added.

## [0.1.0] (initial)

- Single entry point `run.sh`; lattice and MD simulation; optional Qt GUI with Simulation, Lattice, 3D View tabs.
- Core types, units, lattice (volume, min-image), config stub.
