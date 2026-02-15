# Contributing to MATSIMU

## Principles

- **Simplicity first**: one entry point (`./run.sh`), minimal dependencies, clear structure.
- **Dual audience**: code and docs should serve both domain experts and learners.
- **Correctness over speed**: explicit invariants, no silent failures, deterministic behaviour where applicable.
- **SI in core**: all simulation internals in SI; conversions only at I/O boundaries.

## Setup

- Clone the repo; from the project root run `./run.sh` to build and run (CLI if no Qt; GUI if Qt 6.2+ is installed).
- Run tests: `./run.sh --test`.
- See `docs/ARCHITECTURE.md` and `docs/UNITS.md` for design and units.

## Making changes

1. **Code**: Add headers under `include/matsimu/<layer>/`, implementation under `src/<layer>/`. Preserve dependency direction (no cycles); keep one responsibility per header.
2. **Tests**: Add or extend tests in `tests/test_main.cpp` (or new files under `tests/` if you extend the test harness). Cover parameter validation, stability limits, config parsing, and deterministic behaviour where relevant. Run `./run.sh --test`.
3. **Config**: Empty path must return defaults; invalid non-empty path must yield an explicit error (no silent defaults). See `include/matsimu/io/config.hpp` and `docs/ARCHITECTURE.md`.
4. **Docs**: Update `README.md`, `docs/ARCHITECTURE.md`, `docs/UNITS.md`, and `examples/README.md` when you add features or change contracts.
5. **Changelog**: Add a short entry to `CHANGELOG.md` for user-visible changes.

## Submitting

- Prefer small, focused patches.
- Ensure `./run.sh` and `./run.sh --test` succeed before submitting.
