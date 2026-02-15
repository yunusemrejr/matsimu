#pragma once

#include <matsimu/core/types.hpp>
#include <string>    // For std::string
#include <optional>  // For std::optional

namespace matsimu {

/**
 * Lattice: three basis vectors a1, a2, a3 (right-handed).
 * All lattice points R = n1*a1 + n2*a2 + n3*a3.
 * Lengths in SI (m). Volume V = a1 · (a2 × a3).
 */
struct Lattice {
  Real a1[3]{1, 0, 0};
  Real a2[3]{0, 1, 0};
  Real a3[3]{0, 0, 1};

  /// Cell volume (m³).
  Real volume() const;

  /// Minimum-image vector in fractional coordinates (half-box convention).
  void min_image_frac(Real frac[3]) const;

  /// Convert Cartesian coordinates to fractional (direct) coordinates.
  void cartesian_to_fractional(const Real cart[3], Real frac[3]) const;

  /// Convert fractional (direct) coordinates to Cartesian coordinates.
  void fractional_to_cartesian(const Real frac[3], Real cart[3]) const;

  /// Apply periodic boundary conditions to Cartesian coordinates (wrap into box).
  void wrap_cartesian(Real cart[3]) const;

  /// Compute minimum-image displacement vector in Cartesian coordinates.
  void min_image_displacement(const Real r1[3], const Real r2[3], Real dr[3]) const;

  /// Validate lattice: returns an error message if invalid, std::nullopt otherwise.
  std::optional<std::string> validate() const;

  /// Compute reciprocal lattice vectors (b1, b2, b3) where b_i · a_j = 2π δ_ij.
  void reciprocal_vectors(Real b1[3], Real b2[3], Real b3[3]) const;

  /// Check if lattice is orthogonal (a1 along x, a2 along y, a3 along z).
  bool is_orthogonal() const;
};

}  // namespace matsimu
