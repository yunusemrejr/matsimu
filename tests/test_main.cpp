/**
 * Lightweight C++ tests for MATSIMU: parameter validation, stability,
 * lattice, config, deterministic stepping. Run via: ./run.sh --test
 */
#include <matsimu/core/types.hpp>
#include <matsimu/core/units.hpp>
#include <matsimu/io/config.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/sim/simulation.hpp>
#include <matsimu/sim/heat_diffusion.hpp>
#include <matsimu/sim/heat_diffusion_2d.hpp>
#include <matsimu/physics/potential.hpp>
#include <matsimu/physics/thermostat.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>
#include <array>

#define ASSERT(x) do { if (!(x)) { std::cerr << "FAIL: " << #x << " at " << __FILE__ << ":" << __LINE__ << "\n"; return 1; } } while(0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { std::cerr << "FAIL: " << #a << " == " << #b << " at " << __FILE__ << ":" << __LINE__ << " (" << (a) << " != " << (b) << ")\n"; return 1; } } while(0)

namespace {

int test_param_validation() {
  matsimu::SimulationParams p;
  p.dt = 0;
  ASSERT(p.validate().has_value());
  p.dt = 1e-15;
  p.end_time = -1;
  ASSERT(p.validate().has_value());
  p.end_time = 1e-12;
  ASSERT(!p.validate().has_value());
  return 0;
}

int test_heat_stability_limit() {
  matsimu::HeatDiffusionParams hp;
  hp.alpha = 1e-5;
  hp.dx = 1e-3;
  hp.dt = 0.1;  // exceeds stability limit dx²/(2*alpha) = 1e-6/(2e-5) = 0.05
  ASSERT(hp.validate().has_value());
  hp.dt = 0.01;
  ASSERT(!hp.validate().has_value());
  ASSERT(hp.stability_limit() > 0 && std::isfinite(hp.stability_limit()));
  return 0;
}

int test_lattice_volume_minimage() {
  matsimu::Lattice lat;
  matsimu::Real vol = lat.volume();
  ASSERT(std::abs(vol - 1.0) < 1e-10);
  matsimu::Real frac[3] = {0.7, -0.3, 0.1};
  lat.min_image_frac(frac);
  ASSERT(frac[0] >= -0.5 && frac[0] < 0.5);
  ASSERT(frac[1] >= -0.5 && frac[1] < 0.5);
  ASSERT(frac[2] >= -0.5 && frac[2] < 0.5);
  lat.a1[0] = 2.0;
  lat.a2[1] = 2.0;
  lat.a3[2] = 2.0;
  ASSERT(std::abs(lat.volume() - 8.0) < 1e-10);
  return 0;
}

int test_config_empty_path() {
  matsimu::ConfigResult r = matsimu::load_config("");
  ASSERT(r.ok);
  ASSERT(r.params.dt > 0);
  return 0;
}

int test_config_invalid_path() {
  matsimu::ConfigResult r = matsimu::load_config("/nonexistent/path/matsimu.conf");
  ASSERT(!r.ok);
  ASSERT(!r.error.empty());
  return 0;
}

int test_config_parse_failure() {
  std::string path = "/tmp/matsimu_test_invalid.conf";
  {
    std::ofstream f(path);
    f << "dt=not_a_number\n";
  }
  matsimu::ConfigResult r = matsimu::load_config(path);
  std::remove(path.c_str());
  ASSERT(!r.ok);
  ASSERT(!r.error.empty());
  return 0;
}

int test_deterministic_stepping() {
  matsimu::SimulationParams p;
  p.dt = 1e-15;
  p.end_time = 5e-15;
  p.max_steps = 1000;
  matsimu::Simulation s1(p), s2(p);
  ASSERT(s1.is_valid() && s2.is_valid());
  while (s1.step()) {}
  while (s2.step()) {}
  ASSERT_EQ(s1.step_count(), s2.step_count());
  ASSERT(std::abs(s1.time() - s2.time()) < 1e-20);
  return 0;
}

int test_heat_deterministic() {
  matsimu::HeatDiffusionParams hp;
  hp.alpha = 1e-5;
  hp.dx = 1e-3;
  hp.dt = 4e-7;
  hp.end_time = 1e-4;
  hp.max_steps = 1000;
  hp.n_cells = 20;
  matsimu::Simulation s1(hp), s2(hp);
  ASSERT(s1.is_valid() && s2.is_valid());
  while (s1.step()) {}
  while (s2.step()) {}
  ASSERT_EQ(s1.step_count(), s2.step_count());
  ASSERT(std::abs(s1.time() - s2.time()) < 1e-20);
  return 0;
}

int test_heat2d_examples_smoke() {
  matsimu::HeatDiffusion2DParams hot_center;
  hot_center.alpha = 1.11e-4;
  hot_center.dx = 0.1 / 80.0;
  hot_center.nx = 80;
  hot_center.ny = 80;
  hot_center.T_boundary = 300.0;
  hot_center.T_hot = 1200.0;
  hot_center.ic = matsimu::HeatIC2D::HotCenter;
  hot_center.hot_radius_frac = 0.10;
  hot_center.end_time = 0.0;
  hot_center.max_steps = 2000;
  hot_center.dt = 0.85 * hot_center.stability_limit();
  matsimu::Simulation sh(hot_center);
  ASSERT(sh.is_valid());
  ASSERT(sh.heat_2d_model() != nullptr);

  for (int i = 0; i < 200; ++i) {
    ASSERT(sh.step());
  }
  const auto* mh = sh.heat_2d_model();
  ASSERT(mh != nullptr);
  ASSERT(mh->temperature().size() == mh->nx() * mh->ny());
  for (matsimu::Real t : mh->temperature()) {
    ASSERT(std::isfinite(t));
    ASSERT(t >= 0.0);
  }

  matsimu::HeatDiffusion2DParams quench = hot_center;
  quench.alpha = 1.172e-5;
  quench.dx = 0.05 / 80.0;
  quench.ic = matsimu::HeatIC2D::UniformHot;
  quench.dt = 0.85 * quench.stability_limit();
  matsimu::Simulation sq(quench);
  ASSERT(sq.is_valid());
  ASSERT(sq.heat_2d_model() != nullptr);
  for (int i = 0; i < 200; ++i) {
    ASSERT(sq.step());
  }
  const auto* mq = sq.heat_2d_model();
  ASSERT(mq != nullptr);
  for (matsimu::Real t : mq->temperature()) {
    ASSERT(std::isfinite(t));
    ASSERT(t >= 0.0);
  }
  return 0;
}

int test_thermal_shock_like_md_smoke() {
  matsimu::SimulationParams p;
  p.dx = 1e-9;
  p.end_time = 0.0;
  p.max_steps = 5000;
  p.dt = 1e-15;
  p.temperature = 650.0;
  p.cutoff = 1.1e-9;
  p.use_neighbor_list = true;
  p.neighbor_skin = 2.5e-10;

  matsimu::Simulation sim(p);
  ASSERT(sim.is_valid());

  matsimu::Lattice lat;
  lat.a1[0] = 10.0e-9; lat.a1[1] = 0.0;     lat.a1[2] = 0.0;
  lat.a2[0] = 0.0;     lat.a2[1] = 10.0e-9; lat.a2[2] = 0.0;
  lat.a3[0] = 0.0;     lat.a3[1] = 0.0;     lat.a3[2] = 10.0e-9;
  sim.set_lattice(lat);
  sim.set_potential(std::make_shared<matsimu::LennardJones>(1.654e-21, 3.405e-10, 1.1e-9));
  sim.set_thermostat(std::make_shared<matsimu::VelocityRescaleThermostat>(650.0, 8e-13));

  auto& ps = sim.system();
  ps.clear();
  ps.reserve(260);

  std::mt19937 rng(1337u);
  const matsimu::Real kBoltzmann = 1.380649e-23;
  const matsimu::Real sigma_v = std::sqrt(kBoltzmann * 650.0 / 6.63e-26);
  std::normal_distribution<matsimu::Real> thermal(0.0, sigma_v);
  auto add_particle = [&](matsimu::Real px, matsimu::Real py, matsimu::Real pz, matsimu::Real drift_x) {
    matsimu::Particle part;
    part.mass = 6.63e-26;
    part.pos[0] = px; part.pos[1] = py; part.pos[2] = pz;
    part.vel[0] = drift_x + thermal(rng);
    part.vel[1] = thermal(rng);
    part.vel[2] = thermal(rng);
    ps.add_particle(part);
  };

  const matsimu::Real spacing = 3.7e-10;
  const matsimu::Real left_origin_x = 1.7e-9;
  const matsimu::Real right_origin_x = 6.4e-9;
  const matsimu::Real origin_y = 3.5e-9;
  const matsimu::Real origin_z = 3.5e-9;
  const int n = 5;

  for (int ix = 0; ix < n; ++ix) {
    for (int iy = 0; iy < n; ++iy) {
      for (int iz = 0; iz < n; ++iz) {
        add_particle(left_origin_x + spacing * ix,
                     origin_y + spacing * iy,
                     origin_z + spacing * iz,
                     180.0);
        add_particle(right_origin_x + spacing * ix,
                     origin_y + spacing * iy,
                     origin_z + spacing * iz,
                     -180.0);
      }
    }
  }

  if (ps.size() < 40) {
    std::cerr << "FAIL: ps.size() = " << ps.size() << " (expected >= 40) at " << __FILE__ << ":" << __LINE__ << "\n";
    return 1;
  }
  sim.initialize();

  for (int i = 0; i < 300; ++i) {
    ASSERT(sim.step());
    ASSERT(std::isfinite(sim.time()));
    ASSERT(sim.error_message().empty());
  }
  for (const auto& part : sim.system().particles()) {
    ASSERT(part.mass > 0.0 && std::isfinite(part.mass));
    for (int d = 0; d < 3; ++d) {
      ASSERT(std::isfinite(part.pos[d]));
      ASSERT(std::isfinite(part.vel[d]));
      ASSERT(std::isfinite(part.force[d]));
    }
  }
  return 0;
}

}  // namespace

int main() {
  int (*tests[])() = {
    test_param_validation,
    test_heat_stability_limit,
    test_lattice_volume_minimage,
    test_config_empty_path,
    test_config_invalid_path,
    test_config_parse_failure,
    test_deterministic_stepping,
    test_heat_deterministic,
    test_heat2d_examples_smoke,
    test_thermal_shock_like_md_smoke,
  };
  for (auto run : tests) {
    if (run() != 0) return 1;
  }
  std::cout << "All tests passed.\n";
  return 0;
}
