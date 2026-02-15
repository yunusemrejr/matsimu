#include <matsimu/lattice/lattice.hpp>
#include <cmath> // For std::fabs, std::isfinite

namespace matsimu {

Real Lattice::volume() const {
  Real cross[3] = {
    a2[1] * a3[2] - a2[2] * a3[1],
    a2[2] * a3[0] - a2[0] * a3[2],
    a2[0] * a3[1] - a2[1] * a3[0]
  };
  return a1[0] * cross[0] + a1[1] * cross[1] + a1[2] * cross[2];
}

void Lattice::min_image_frac(Real frac[3]) const {
  for (int i = 0; i < 3; ++i) {
    while (frac[i] >= 0.5) frac[i] -= 1.0;
    while (frac[i] < -0.5) frac[i] += 1.0;
  }
}

std::optional<std::string> Lattice::validate() const {
  // Check if all components are finite
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(a1[i])) return "Lattice vector a1 contains non-finite components.";
    if (!std::isfinite(a2[i])) return "Lattice vector a2 contains non-finite components.";
    if (!std::isfinite(a3[i])) return "Lattice vector a3 contains non-finite components.";
  }

  Real vol = volume();
  // Check if volume is finite and non-zero (linearly independent vectors)
  if (!std::isfinite(vol)) {
    return "Lattice volume is non-finite, indicating invalid basis vectors.";
  }
  if (std::fabs(vol) < std::numeric_limits<Real>::epsilon()) {
    return "Lattice vectors are linearly dependent (volume is zero), forming a degenerate lattice.";
  }

  return std::nullopt; // Lattice is valid
}

void Lattice::cartesian_to_fractional(const Real cart[3], Real frac[3]) const {
  // frac = A^{-1} * cart, where A = [a1 a2 a3] is the lattice matrix
  // Using Cramer's rule for the inverse
  Real vol = volume();
  if (std::fabs(vol) < std::numeric_limits<Real>::epsilon()) {
    frac[0] = frac[1] = frac[2] = 0.0;
    return;
  }
  Real inv_vol = 1.0 / vol;

  // Compute cross products for reciprocal vectors (without 2π factor)
  Real a2_cross_a3[3] = {
    a2[1] * a3[2] - a2[2] * a3[1],
    a2[2] * a3[0] - a2[0] * a3[2],
    a2[0] * a3[1] - a2[1] * a3[0]
  };
  Real a3_cross_a1[3] = {
    a3[1] * a1[2] - a3[2] * a1[1],
    a3[2] * a1[0] - a3[0] * a1[2],
    a3[0] * a1[1] - a3[1] * a1[0]
  };
  Real a1_cross_a2[3] = {
    a1[1] * a2[2] - a1[2] * a2[1],
    a1[2] * a2[0] - a1[0] * a2[2],
    a1[0] * a2[1] - a1[1] * a2[0]
  };

  frac[0] = inv_vol * (a2_cross_a3[0] * cart[0] + a2_cross_a3[1] * cart[1] + a2_cross_a3[2] * cart[2]);
  frac[1] = inv_vol * (a3_cross_a1[0] * cart[0] + a3_cross_a1[1] * cart[1] + a3_cross_a1[2] * cart[2]);
  frac[2] = inv_vol * (a1_cross_a2[0] * cart[0] + a1_cross_a2[1] * cart[1] + a1_cross_a2[2] * cart[2]);
}

void Lattice::fractional_to_cartesian(const Real frac[3], Real cart[3]) const {
  // cart = A * frac = frac[0] * a1 + frac[1] * a2 + frac[2] * a3
  cart[0] = frac[0] * a1[0] + frac[1] * a2[0] + frac[2] * a3[0];
  cart[1] = frac[0] * a1[1] + frac[1] * a2[1] + frac[2] * a3[1];
  cart[2] = frac[0] * a1[2] + frac[1] * a2[2] + frac[2] * a3[2];
}

void Lattice::wrap_cartesian(Real cart[3]) const {
  Real frac[3];
  cartesian_to_fractional(cart, frac);

  // Wrap fractional coordinates to [0, 1)
  for (int i = 0; i < 3; ++i) {
    frac[i] = frac[i] - std::floor(frac[i]);
  }

  fractional_to_cartesian(frac, cart);
}

void Lattice::min_image_displacement(const Real r1[3], const Real r2[3], Real dr[3]) const {
  // Compute displacement in Cartesian
  dr[0] = r2[0] - r1[0];
  dr[1] = r2[1] - r1[1];
  dr[2] = r2[2] - r1[2];

  // Convert to fractional, apply min image, convert back
  Real frac[3];
  cartesian_to_fractional(dr, frac);
  min_image_frac(frac);
  fractional_to_cartesian(frac, dr);
}

void Lattice::reciprocal_vectors(Real b1[3], Real b2[3], Real b3[3]) const {
  Real vol = volume();
  if (std::fabs(vol) < std::numeric_limits<Real>::epsilon()) {
    b1[0] = b1[1] = b1[2] = 0.0;
    b2[0] = b2[1] = b2[2] = 0.0;
    b3[0] = b3[1] = b3[2] = 0.0;
    return;
  }
  Real factor = 2.0 * M_PI / vol;

  // b1 = 2π/V * (a2 × a3)
  b1[0] = factor * (a2[1] * a3[2] - a2[2] * a3[1]);
  b1[1] = factor * (a2[2] * a3[0] - a2[0] * a3[2]);
  b1[2] = factor * (a2[0] * a3[1] - a2[1] * a3[0]);

  // b2 = 2π/V * (a3 × a1)
  b2[0] = factor * (a3[1] * a1[2] - a3[2] * a1[1]);
  b2[1] = factor * (a3[2] * a1[0] - a3[0] * a1[2]);
  b2[2] = factor * (a3[0] * a1[1] - a3[1] * a1[0]);

  // b3 = 2π/V * (a1 × a2)
  b3[0] = factor * (a1[1] * a2[2] - a1[2] * a2[1]);
  b3[1] = factor * (a1[2] * a2[0] - a1[0] * a2[2]);
  b3[2] = factor * (a1[0] * a2[1] - a1[1] * a2[0]);
}

bool Lattice::is_orthogonal() const {
  constexpr Real tol = 1e-10;
  // Check if off-diagonal elements are zero (a1 along x, a2 along y, a3 along z)
  if (std::fabs(a1[1]) > tol || std::fabs(a1[2]) > tol) return false;
  if (std::fabs(a2[0]) > tol || std::fabs(a2[2]) > tol) return false;
  if (std::fabs(a3[0]) > tol || std::fabs(a3[1]) > tol) return false;
  return true;
}

}  // namespace matsimu
