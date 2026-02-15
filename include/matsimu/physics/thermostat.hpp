#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/physics/particle.hpp>
#include <cmath>

namespace matsimu {

/**
 * Thermostat for temperature control in molecular dynamics.
 * 
 * Thermostats modify velocities to maintain or reach a target temperature.
 */
class Thermostat {
public:
    virtual ~Thermostat() = default;
    
    /// Apply thermostat to particle system
    virtual void apply(ParticleSystem& system, Real dt) = 0;
    
    /// Get target temperature [K]
    virtual Real target_temperature() const = 0;
    
    /// Set target temperature [K]
    virtual void set_target_temperature(Real T) = 0;
};

/**
 * Velocity rescaling thermostat (Berendsen-like, simple).
 * 
 * Scales all velocities by a factor to match target temperature.
 * Fast but does not sample the canonical ensemble correctly.
 * Good for equilibration, not for production.
 */
class VelocityRescaleThermostat : public Thermostat {
public:
    /**
     * Construct velocity rescaling thermostat.
     * 
     * @param target_T Target temperature [K]
     * @param tau Relaxation time [s] (smaller = stronger coupling)
     */
    VelocityRescaleThermostat(Real target_T, Real tau);
    
    void apply(ParticleSystem& system, Real dt) override;
    
    Real target_temperature() const override { return target_T_; }
    void set_target_temperature(Real T) override { target_T_ = T; }
    
    Real tau() const { return tau_; }
    void set_tau(Real tau) { tau_ = tau; }

private:
    Real target_T_;
    Real tau_;
};

/**
 * Andersen thermostat (stochastic collisions).
 * 
 * Randomly selects particles and assigns new velocities
 * from Maxwell-Boltzmann distribution.
 * Samples canonical ensemble correctly.
 */
class AndersenThermostat : public Thermostat {
public:
    /**
     * Construct Andersen thermostat.
     * 
     * @param target_T Target temperature [K]
     * @param nu Collision frequency [1/s]
     * @param seed Random seed (0 = use random device)
     */
    AndersenThermostat(Real target_T, Real nu, unsigned seed = 0);
    
    void apply(ParticleSystem& system, Real dt) override;
    
    Real target_temperature() const override { return target_T_; }
    void set_target_temperature(Real T) override { target_T_ = T; }
    
    Real collision_frequency() const { return nu_; }
    void set_collision_frequency(Real nu) { nu_ = nu; }

private:
    Real target_T_;
    Real nu_;
    unsigned seed_;
    
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * No-op thermostat (NVE ensemble - constant energy).
 */
class NullThermostat : public Thermostat {
public:
    NullThermostat() = default;
    
    void apply(ParticleSystem&, Real) override { /* do nothing */ }
    
    Real target_temperature() const override { return 0.0; }
    void set_target_temperature(Real) override { /* ignore */ }
};

} // namespace matsimu
