#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/alloc/bounded_allocator.hpp>
#include <vector>
#include <memory>

namespace matsimu {

/**
 * Single particle (atom) state in 3D.
 * Stores position, velocity, and force vectors.
 */
struct Particle {
    Real pos[3] = {0.0, 0.0, 0.0};   // Position [m]
    Real vel[3] = {0.0, 0.0, 0.0};   // Velocity [m/s]
    Real force[3] = {0.0, 0.0, 0.0}; // Force [N]
    Real mass = 1.0;                  // Mass [kg]
    
    /// Zero out the force vector
    void clear_force() {
        force[0] = force[1] = force[2] = 0.0;
    }
    
    /// Add to force vector
    void add_force(Real fx, Real fy, Real fz) {
        force[0] += fx;
        force[1] += fy;
        force[2] += fz;
    }
};

/**
 * Collection of particles with simulation state.
 * Resource-aware: uses vector with bounded allocator support.
 */
class ParticleSystem {
public:
    using ParticleAllocator = bounded_allocator<Particle>;
    
    /// Construct with a default limit of 1GB for particles (approx 12M particles)
    explicit ParticleSystem(std::size_t max_bytes = 1024 * 1024 * 1024) 
        : particles_(ParticleAllocator(max_bytes)) {}
        
    /// Construct with initial size and a default limit
    ParticleSystem(std::size_t n, std::size_t max_bytes = 1024 * 1024 * 1024) 
        : particles_(n, Particle(), ParticleAllocator(max_bytes)) {}

    /// Add a particle to the system
    void add_particle(const Particle& p) {
        particles_.push_back(p);
    }
    
    /// Reserve space for n particles
    void reserve(std::size_t n) {
        particles_.reserve(n);
    }
    
    /// Access particle by index
    Particle& operator[](std::size_t i) { return particles_[i]; }
    const Particle& operator[](std::size_t i) const { return particles_[i]; }
    
    /// Number of particles
    std::size_t size() const { return particles_.size(); }
    
    /// Check if empty
    bool empty() const { return particles_.empty(); }
    
    /// Clear all particles
    void clear() { particles_.clear(); }
    
    /// Clear all forces (call before force calculation)
    void clear_forces();
    
    /// Calculate total kinetic energy [J]
    Real kinetic_energy() const;
    
    /// Calculate temperature from kinetic energy [K]
    /// For N particles in 3D: T = 2*E_kin / (3*N*k_B)
    Real temperature() const;
    
    /// Get center of mass position
    void center_of_mass(Real com[3]) const;
    
    /// Remove center of mass velocity (drift correction)
    void zero_com_velocity();
    
    /// Apply periodic boundary conditions using lattice
    void apply_pbc(const Lattice& lattice);
    
    /// Access underlying container
    const std::vector<Particle, ParticleAllocator>& particles() const { return particles_; }
    std::vector<Particle, ParticleAllocator>& particles() { return particles_; }
    
private:
    std::vector<Particle, ParticleAllocator> particles_;
};


} // namespace matsimu
