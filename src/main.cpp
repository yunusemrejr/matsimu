#include <matsimu/core/types.hpp>
#include <matsimu/io/config.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/sim/simulation.hpp>
#include <iostream>
#include <string>
#include <cstring>

#ifdef MATSIMU_USE_QT
#include <QApplication>
#include <QMetaObject>
#include <matsimu/ui/main_window.hpp>
#endif

namespace {

const char* get_arg(int argc, char* argv[], const char* name) {
  for (int i = 1; i + 1 < (argc); ++i)
    if (std::strcmp(argv[i], name) == 0) return argv[i + 1];
  return nullptr;
}

#ifdef MATSIMU_USE_QT
bool has_flag(int argc, char* argv[], const char* name) {
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], name) == 0) return true;
  return false;
}
#endif

void run_lattice_example() {
  matsimu::Lattice lat;
  std::cout << "Lattice volume (default cubic 1 m): " << lat.volume() << " m³\n";
  matsimu::Real frac[3] = {0.7, -0.3, 0.1};
  lat.min_image_frac(frac);
  std::cout << "Min-image frac: " << frac[0] << ", " << frac[1] << ", " << frac[2] << "\n";
}

void run_heat_example() {
  matsimu::HeatDiffusionParams params;
  params.alpha = 1e-5;
  params.dx = 1e-3;
  params.dt = 4e-7;  // below stability limit dx²/(2α) = 5e-5
  params.end_time = 1e-3;
  params.max_steps = 10000;
  params.n_cells = 50;
  matsimu::Simulation sim(params);
  if (!sim.is_valid()) {
    std::cerr << "Error: " << sim.error_message() << "\n";
    return;
  }
  std::cout << "Heat diffusion: alpha=" << params.alpha << " m²/s, dx=" << params.dx
            << " m, dt=" << params.dt << " s, end_time=" << params.end_time << " s\n";
  std::size_t steps = 0;
  while (sim.step()) ++steps;
  std::cout << "Finished at t=" << sim.time() << " s, steps=" << sim.step_count() << "\n";
}

#ifndef MATSIMU_USE_QT
void run_default_cli(const matsimu::SimulationParams& params) {
  matsimu::Simulation sim(params);
  if (!sim.is_valid()) {
    std::cerr << "Error: " << sim.error_message() << "\n";
    return;
  }
  std::cout << "Running simulation: dt=" << params.dt << " s, end_time=" << params.end_time
            << " s, max_steps=" << params.max_steps << "\n";
  std::size_t steps = 0;
  while (sim.step()) ++steps;
  std::cout << "Done. t=" << sim.time() << " s, steps=" << sim.step_count();
  if (!sim.error_message().empty())
    std::cout << ", error: " << sim.error_message();
  std::cout << "\n";
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
  const char* ex = get_arg(argc, argv, "--example");
  if (ex && std::string(ex) == "lattice") {
    run_lattice_example();
    return 0;
  }
  if (ex && std::string(ex) == "heat") {
    run_heat_example();
    return 0;
  }

#ifdef MATSIMU_USE_QT
  QApplication app(argc, argv);
  const bool auto_run = has_flag(argc, argv, "--autorun");
  matsimu::MainWindow win;
  win.show();
  if (auto_run) {
    QMetaObject::invokeMethod(&win, "on_run", Qt::QueuedConnection);
  }
  return app.exec();
#else
  matsimu::SimulationParams params;
  const char* config_path = get_arg(argc, argv, "--config");
  if (config_path) {
    matsimu::ConfigResult res = matsimu::load_config(config_path);
    if (!res.ok) {
      std::cerr << "Config error: " << res.error << "\n";
      return 1;
    }
    params = res.params;
  } else {
    params.end_time = 2.0 * params.dt;
    params.max_steps = 1000;
  }
  run_default_cli(params);
  return 0;
#endif
}
