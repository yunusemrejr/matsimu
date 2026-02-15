# MATSIMU unit system

## Convention

- **SI base**: length [m], mass [kg], time [s], temperature [K], current [A].
- **Derived**: energy [J], pressure/stress [Pa], thermal diffusivity [m²/s], etc.
- All simulation internals use this system. **No silent mixing** of nm/m, eV/J, fs/s in core code.

## Conversions

- Conversions only at **I/O** or **external library** boundaries.
- Document any conversion (e.g. "input file in nm → convert to m on load").
- Crystallography: 1 Å = 0.1 nm = 10⁻¹⁰ m; use explicitly when needed.

## Constants

- See `include/matsimu/core/units.hpp` for symbolic constants (m, s, kg, K, J, Pa).
- Physical constants (e.g. Boltzmann) should be added in a single place with units stated.

## Heat diffusion

- Thermal diffusivity α [m²/s], grid spacing dx [m], time step dt [s], temperature [K].
- Explicit 1D stability: dt ≤ dx²/(2α); enforced in `HeatDiffusionParams::validate()`.

## Validation

- Material parameters (e.g. Young’s modulus E > 0, Poisson 0 < ν < 0.5) must be validated on load; fail fast on invalid values.
- Time step must respect stability limits (CFL, diffusion); checked at runtime in model param validation.
