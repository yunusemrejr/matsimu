#include <matsimu/physics/integrator.hpp>

namespace matsimu {

void VelocityVerlet::step1(ParticleSystem& system) const {
    for (auto& p : system.particles()) {
        // v(t+dt/2) = v(t) + 0.5*dt*a(t)
        Real ax = p.force[0] / p.mass;
        Real ay = p.force[1] / p.mass;
        Real az = p.force[2] / p.mass;
        
        p.vel[0] += half_dt_ * ax;
        p.vel[1] += half_dt_ * ay;
        p.vel[2] += half_dt_ * az;
        
        // r(t+dt) = r(t) + dt*v(t+dt/2)
        p.pos[0] += dt_ * p.vel[0];
        p.pos[1] += dt_ * p.vel[1];
        p.pos[2] += dt_ * p.vel[2];
    }
}

void VelocityVerlet::step2(ParticleSystem& system) const {
    for (auto& p : system.particles()) {
        // v(t+dt) = v(t+dt/2) + 0.5*dt*a(t+dt)
        Real ax = p.force[0] / p.mass;
        Real ay = p.force[1] / p.mass;
        Real az = p.force[2] / p.mass;
        
        p.vel[0] += half_dt_ * ax;
        p.vel[1] += half_dt_ * ay;
        p.vel[2] += half_dt_ * az;
    }
}

void VelocityVerlet::integrate(ParticleSystem& system,
                               std::function<void(ParticleSystem&)> compute_forces) const {
    // First half-step
    step1(system);
    
    // Compute new forces
    system.clear_forces();
    compute_forces(system);
    
    // Second half-step
    step2(system);
}

void EulerIntegrator::step(ParticleSystem& system) const {
    for (auto& p : system.particles()) {
        // Update velocity
        Real ax = p.force[0] / p.mass;
        Real ay = p.force[1] / p.mass;
        Real az = p.force[2] / p.mass;
        
        p.vel[0] += dt_ * ax;
        p.vel[1] += dt_ * ay;
        p.vel[2] += dt_ * az;
        
        // Update position
        p.pos[0] += dt_ * p.vel[0];
        p.pos[1] += dt_ * p.vel[1];
        p.pos[2] += dt_ * p.vel[2];
    }
}

// TimeStepValidation implementation

Real TimeStepValidation::estimate_characteristic_time(const ParticleSystem& system) {
    // Estimate characteristic time from particle properties
    // For a harmonic oscillator: tau = 2*pi*sqrt(m/k)
    // We estimate from typical mass and velocity
    
    if (system.empty()) return 1.0;
    
    Real min_mass = std::numeric_limits<Real>::max();
    Real max_vel = 0.0;
    
    for (const auto& p : system.particles()) {
        if (p.mass > 0.0 && p.mass < min_mass) {
            min_mass = p.mass;
        }
        Real v2 = p.vel[0]*p.vel[0] + p.vel[1]*p.vel[1] + p.vel[2]*p.vel[2];
        Real v = std::sqrt(v2);
        if (v > max_vel) {
            max_vel = v;
        }
    }
    
    if (max_vel < 1e-10) {
        // If velocities are very small, use a default based on mass
        // Typical MD: dt ~ 1 fs for atomic masses
        return 1e-14; // ~0.01 ps in SI
    }
    
    // Characteristic time: time to travel a typical distance at max velocity
    // Use a typical atomic spacing as reference (~1 Angstrom)
    constexpr Real typical_distance = 1e-10; // 1 Angstrom
    return typical_distance / max_vel;
}

bool TimeStepValidation::is_stable(Real dt, const ParticleSystem& system) {
    Real tau = estimate_characteristic_time(system);
    // Conservative criterion: dt should be less than tau/10
    return dt < tau / 10.0;
}

Real TimeStepValidation::recommended_max_dt(const ParticleSystem& system) {
    Real tau = estimate_characteristic_time(system);
    // Recommend dt = tau / 20 for good accuracy
    return tau / 20.0;
}

void TimeStepValidation::validate_dt(Real dt, const ParticleSystem& system, bool warn) {
    Real tau = estimate_characteristic_time(system);
    Real max_dt = tau / 10.0;
    
    if (dt > max_dt && warn) {
        // In a real application, this would use a logging system
        // For now, we just note the potential issue
        // std::cerr << "Warning: Time step " << dt << " may be too large. "
        //           << "Recommended max: " << max_dt << "\n";
    }
}

} // namespace matsimu
