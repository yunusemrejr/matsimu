#include <matsimu/physics/potential.hpp>
#include <algorithm>

namespace matsimu {

// Lennard-Jones implementation
LennardJones::LennardJones(Real epsilon, Real sigma, Real cutoff)
    : epsilon_(epsilon), sigma_(sigma), cutoff_sq_(cutoff * cutoff) {
    sigma_sq_ = sigma_ * sigma_;
    sigma_6_ = sigma_sq_ * sigma_sq_ * sigma_sq_;
    sigma_12_ = sigma_6_ * sigma_6_;
    
    // Calculate shift for continuity at cutoff
    Real r2_inv = sigma_sq_ / cutoff_sq_;
    Real r6_inv = r2_inv * r2_inv * r2_inv;
    Real r12_inv = r6_inv * r6_inv;
    shift_ = 4.0 * epsilon_ * (r12_inv - r6_inv);
}

Real LennardJones::energy(Real r2) const {
    if (r2 >= cutoff_sq_) return 0.0;
    
    Real r2_inv = sigma_sq_ / r2;
    Real r6_inv = r2_inv * r2_inv * r2_inv;
    Real r12_inv = r6_inv * r6_inv;
    
    return 4.0 * epsilon_ * (r12_inv - r6_inv) - shift_;
}

Real LennardJones::force_div_r(Real r2) const {
    if (r2 >= cutoff_sq_) return 0.0;
    
    Real r2_inv = sigma_sq_ / r2;
    Real r6_inv = r2_inv * r2_inv * r2_inv;
    Real r12_inv = r6_inv * r6_inv;
    
    // F = -dU/dr * (r_vec/r)
    // dU/dr = 4*eps * (-12*sigma^12/r^13 + 6*sigma^6/r^7)
    // F/r = -dU/dr / r = 4*eps * (12*sigma^12/r^14 - 6*sigma^6/r^8)
    //     = 24*eps / r^2 * (2*sigma^12/r^12 - sigma^6/r^6)
    //     = 24*eps / sigma^2 * (2*r12_inv - r6_inv) * (sigma^2/r^2)
    return 24.0 * epsilon_ * (2.0 * r12_inv - r6_inv) / r2;
}

// Harmonic potential implementation
HarmonicPotential::HarmonicPotential(Real k, Real r0, Real cutoff)
    : k_(k), r0_(r0), cutoff_sq_(cutoff * cutoff) {}

Real HarmonicPotential::energy(Real r2) const {
    if (r2 >= cutoff_sq_) return 0.0;
    
    Real r = std::sqrt(r2);
    Real dr = r - r0_;
    return 0.5 * k_ * dr * dr;
}

Real HarmonicPotential::force_div_r(Real r2) const {
    if (r2 >= cutoff_sq_) return 0.0;
    
    Real r = std::sqrt(r2);
    Real dr = r - r0_;
    // F = -k*(r-r0) * (r_vec/r)
    // F/r = -k*(r-r0)/r
    return -k_ * dr / r2;
}

// ForceField implementation
Real ForceField::compute_forces(ParticleSystem& system, const Lattice* lattice) const {
    if (!potential_) return 0.0;
    
    system.clear_forces();
    Real epot = 0.0;
    std::size_t n = system.size();
    
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            Real dx[3];
            Real r2 = distance_squared(system[i], system[j], lattice, dx);
            
            if (r2 < potential_->cutoff_squared()) {
                Real e = potential_->energy(r2);
                Real f_div_r = potential_->force_div_r(r2);
                
                epot += e;
                
                // F_ij = f_div_r * r_vec
                Real fx = f_div_r * dx[0];
                Real fy = f_div_r * dx[1];
                Real fz = f_div_r * dx[2];
                
                system[i].add_force(fx, fy, fz);
                system[j].add_force(-fx, -fy, -fz);
            }
        }
    }
    
    return epot;
}

Real ForceField::compute_energy(const ParticleSystem& system, const Lattice* lattice) const {
    if (!potential_) return 0.0;
    
    Real epot = 0.0;
    std::size_t n = system.size();
    Real dx[3];
    
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            Real r2 = distance_squared(system[i], system[j], lattice, dx);
            if (r2 < potential_->cutoff_squared()) {
                epot += potential_->energy(r2);
            }
        }
    }
    
    return epot;
}

Real ForceField::distance_squared(const Particle& p1, const Particle& p2,
                                    const Lattice* lattice, Real dx[3]) {
    dx[0] = p2.pos[0] - p1.pos[0];
    dx[1] = p2.pos[1] - p1.pos[1];
    dx[2] = p2.pos[2] - p1.pos[2];
    
    if (lattice) {
        // Use the lattice's min_image_displacement for proper PBC handling
        // This works for both orthogonal and non-orthogonal lattices
        lattice->min_image_displacement(p1.pos, p2.pos, dx);
    }
    
    return dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2];
}

} // namespace matsimu
