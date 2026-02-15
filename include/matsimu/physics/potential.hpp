#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/physics/particle.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <cmath>

namespace matsimu {

/**
 * Base class for interatomic potentials.
 * All potentials must implement energy and force calculations.
 */
class Potential {
public:
    virtual ~Potential() = default;
    
    /// Calculate pair energy given squared distance
    virtual Real energy(Real r2) const = 0;
    
    /// Calculate pair force magnitude divided by r (F/r = -dU/dr / r)
    /// Returns f/r where F = (f/r) * r_vec
    virtual Real force_div_r(Real r2) const = 0;
    
    /// Get cutoff distance squared
    virtual Real cutoff_squared() const = 0;
    
    /// Calculate cutoff distance
    Real cutoff() const { return std::sqrt(cutoff_squared()); }
};

/**
 * Lennard-Jones 12-6 potential.
 * 
 * U(r) = 4*epsilon * [(sigma/r)^12 - (sigma/r)^6]
 * 
 * Common parameters:
 *   Argon: epsilon = 1.654e-21 J, sigma = 3.405e-10 m
 */
class LennardJones : public Potential {
public:
    LennardJones(Real epsilon, Real sigma, Real cutoff);
    
    Real energy(Real r2) const override;
    Real force_div_r(Real r2) const override;
    Real cutoff_squared() const override { return cutoff_sq_; }
    
    /// Get potential parameters
    Real epsilon() const { return epsilon_; }
    Real sigma() const { return sigma_; }

private:
    Real epsilon_;      // Depth of potential well [J]
    Real sigma_;        // Finite distance where potential is zero [m]
    Real cutoff_sq_;    // Cutoff radius squared [m^2]
    Real shift_;        // Energy shift at cutoff for continuity
    
    // Precomputed powers for efficiency
    Real sigma_sq_;     // sigma^2
    Real sigma_6_;      // sigma^6
    Real sigma_12_;     // sigma^12
};

/**
 * Harmonic (spring) potential for bonded interactions.
 * 
 * U(r) = 0.5 * k * (r - r0)^2
 */
class HarmonicPotential : public Potential {
public:
    HarmonicPotential(Real k, Real r0, Real cutoff);
    
    Real energy(Real r2) const override;
    Real force_div_r(Real r2) const override;
    Real cutoff_squared() const override { return cutoff_sq_; }
    
    Real k() const { return k_; }
    Real r0() const { return r0_; }

private:
    Real k_;          // Spring constant [N/m]
    Real r0_;         // Equilibrium distance [m]
    Real cutoff_sq_;  // Cutoff radius squared [m^2]
};

/**
 * Force field calculator for pairwise interactions.
 * Computes forces and potential energy for all particle pairs.
 */
class ForceField {
public:
    explicit ForceField(std::shared_ptr<Potential> potential)
        : potential_(std::move(potential)) {}
    
    /// Set the potential
    void set_potential(std::shared_ptr<Potential> potential) {
        potential_ = std::move(potential);
    }
    
    /// Get the potential
    Potential* potential() const { return potential_.get(); }
    
    /**
     * Calculate all pairwise forces and return total potential energy.
     * Applies minimum image convention for periodic boundaries.
     * 
     * @param system Particle system
     * @param lattice Lattice for periodic boundaries (nullptr = non-periodic)
     * @return Total potential energy [J]
     */
    Real compute_forces(ParticleSystem& system, const Lattice* lattice = nullptr) const;
    
    /**
     * Calculate potential energy only (no forces).
     * Faster when only energy is needed.
     */
    Real compute_energy(const ParticleSystem& system, const Lattice* lattice = nullptr) const;

private:
    std::shared_ptr<Potential> potential_;
    
    /// Squared distance between particles with optional PBC
    static Real distance_squared(const Particle& p1, const Particle& p2,
                                  const Lattice* lattice,
                                  Real dx[3]); // Output: displacement vector
};

} // namespace matsimu
