/**
 * Standalone heat-diffusion example (same behaviour as ./run.sh --example heat).
 * Compile with: g++ -I ../include -std=c++17 -o heat_example heat_diffusion_example.cpp \
 *   ../src/sim/heat_diffusion.cpp ../src/sim/simulation.cpp ...
 * Or run via main: ./run.sh --example heat
 */
#include <matsimu/sim/simulation.hpp>
#include <matsimu/sim/heat_diffusion.hpp>
#include <iostream>

int main() {
  matsimu::HeatDiffusionParams params;
  params.alpha = 1e-5;
  params.dx = 1e-3;
  params.dt = 4e-7;  // below stability limit dx²/(2α)
  params.end_time = 1e-3;
  params.max_steps = 10000;
  params.n_cells = 50;

  matsimu::Simulation sim(params);
  if (!sim.is_valid()) {
    std::cerr << "Error: " << sim.error_message() << "\n";
    return 1;
  }
  std::cout << "Heat diffusion: alpha=" << params.alpha << " m²/s, dx=" << params.dx
            << " m, dt=" << params.dt << " s, end_time=" << params.end_time << " s\n";
  while (sim.step()) {}
  std::cout << "Finished at t=" << sim.time() << " s, steps=" << sim.step_count() << "\n";
  return 0;
}
