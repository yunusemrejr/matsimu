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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

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
  };
  for (auto run : tests) {
    if (run() != 0) return 1;
  }
  std::cout << "All tests passed.\n";
  return 0;
}
