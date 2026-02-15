#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/physics/particle.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <cmath>
#include <functional>

namespace matsimu {

/**
 * Velocity Verlet integrator for molecular dynamics.
 * 
 * The Velocity Verlet algorithm is symplectic and time-reversible,
 * making it ideal for conservative Hamiltonian systems.
 * 
 * Algorithm:
 *   1. v(t+dt/2) = v(t) + 0.5*dt*a(t)
 *   2. r(t+dt) = r(t) + dt*v(t+dt/2)
 *   3. Calculate F(t+dt) and a(t+dt) = F(t+dt)/m
 *   4. v(t+dt) = v(t+dt/2) + 0.5*dt*a(t+dt)
 */
class VelocityVerlet {
public:
    explicit VelocityVerlet(Real dt) : dt_(dt), half_dt_(0.5 * dt) {}
    
    /// Set time step
    void set_dt(Real dt) {
        dt_ = dt;
        half_dt_ = 0.5 * dt;
    }
    
    Real dt() const { return dt_; }
    
    /**
     * First half-step: update velocities and positions.
     * Call this before computing new forces.
     */
    void step1(ParticleSystem& system) const;
    
    /**
     * Second half-step: update velocities with new forces.
     * Call this after computing forces at new positions.
     */
    void step2(ParticleSystem& system) const;
    
    /**
     * Full integration step (convenience method).
     * Note: You must compute forces between step1 and step2.
     */
    void integrate(ParticleSystem& system,
                   std::function<void(ParticleSystem&)> compute_forces) const;

private:
    Real dt_;
    Real half_dt_;
};

/**
 * Simple Euler integrator (for comparison/testing only).
 * Not recommended for production MD - use VelocityVerlet instead.
 */
class EulerIntegrator {
public:
    explicit EulerIntegrator(Real dt) : dt_(dt) {}
    
    void set_dt(Real dt) { dt_ = dt; }
    Real dt() const { return dt_; }
    
    /// Full Euler step (less accurate, not symplectic)
    void step(ParticleSystem& system) const;

private:
    Real dt_;
};

/**
 * Time step validation and stability utilities.
 * 
 * For molecular dynamics, the time step must be small enough to resolve
 * the fastest motions in the system (typically bond vibrations).
 */
namespace TimeStepValidation {
    /// Estimate characteristic timescale from particle masses and typical forces
    Real estimate_characteristic_time(const ParticleSystem& system);
    
    /// Check if time step is stable (dt < tau/10 is typically safe)
    bool is_stable(Real dt, const ParticleSystem& system);
    
    /// Get recommended maximum time step
    Real recommended_max_dt(const ParticleSystem& system);
    
    /// Validate and optionally warn if time step may be too large
    void validate_dt(Real dt, const ParticleSystem& system, bool warn = true);
}

} // namespace matsimu
