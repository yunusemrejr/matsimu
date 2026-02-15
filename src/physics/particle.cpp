#include <matsimu/physics/particle.hpp>
#include <algorithm>
#include <numeric>

namespace matsimu {

// Boltzmann constant [J/K]
constexpr Real kB = 1.380649e-23;

void ParticleSystem::clear_forces() {
    for (auto& p : particles_) {
        p.clear_force();
    }
}

Real ParticleSystem::kinetic_energy() const {
    Real ekin = 0.0;
    for (const auto& p : particles_) {
        Real v2 = p.vel[0] * p.vel[0] + p.vel[1] * p.vel[1] + p.vel[2] * p.vel[2];
        ekin += 0.5 * p.mass * v2;
    }
    return ekin;
}

Real ParticleSystem::temperature() const {
    std::size_t n = particles_.size();
    if (n <= 1) return 0.0;
    
    Real ekin = kinetic_energy();
    // T = 2*E_kin / (dof * k_B)
    // For N particles with fixed COM, dof = 3*N - 3
    Real dof = 3.0 * static_cast<Real>(n) - 3.0;
    return (2.0 * ekin) / (dof * kB);
}

void ParticleSystem::center_of_mass(Real com[3]) const {
    com[0] = com[1] = com[2] = 0.0;
    Real total_mass = 0.0;
    
    for (const auto& p : particles_) {
        com[0] += p.mass * p.pos[0];
        com[1] += p.mass * p.pos[1];
        com[2] += p.mass * p.pos[2];
        total_mass += p.mass;
    }
    
    if (total_mass > 0.0) {
        com[0] /= total_mass;
        com[1] /= total_mass;
        com[2] /= total_mass;
    }
}

void ParticleSystem::zero_com_velocity() {
    Real com_vel[3] = {0.0, 0.0, 0.0};
    Real total_mass = 0.0;
    
    for (const auto& p : particles_) {
        com_vel[0] += p.mass * p.vel[0];
        com_vel[1] += p.mass * p.vel[1];
        com_vel[2] += p.mass * p.vel[2];
        total_mass += p.mass;
    }
    
    if (total_mass > 0.0) {
        com_vel[0] /= total_mass;
        com_vel[1] /= total_mass;
        com_vel[2] /= total_mass;
    }
    
    for (auto& p : particles_) {
        p.vel[0] -= com_vel[0];
        p.vel[1] -= com_vel[1];
        p.vel[2] -= com_vel[2];
    }
}

void ParticleSystem::apply_pbc(const Lattice& lattice) {
    // Use the lattice's wrap_cartesian method for proper PBC handling
    // This works for both orthogonal and non-orthogonal lattices
    for (auto& p : particles_) {
        lattice.wrap_cartesian(p.pos);
    }
}

} // namespace matsimu
