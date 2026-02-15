#include <matsimu/ui/main_window.hpp>
#include <matsimu/ui/simulation_tab.hpp>
#include <matsimu/ui/lattice_tab.hpp>
#include <matsimu/ui/view_3d_tab.hpp>
#include <matsimu/sim/simulation.hpp>
#include <matsimu/sim/heat_diffusion_2d.hpp>
#include <matsimu/physics/potential.hpp>
#include <matsimu/physics/thermostat.hpp>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QElapsedTimer>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace matsimu {

namespace {

constexpr Real kBoltzmann = 1.380649e-23;

Real thermal_velocity_stddev(Real temperature_k, Real mass_kg) {
  if (temperature_k <= 0.0 || mass_kg <= 0.0) return 0.0;
  return std::sqrt(kBoltzmann * temperature_k / mass_kg);
}

void populate_argon_crystal(Simulation& sim, const Lattice& lat) {
  sim.set_lattice(lat);

  auto& ps = sim.system();
  ps.clear();
  ps.reserve(1600);

  std::mt19937 rng(42u);
  std::normal_distribution<Real> jitter(0.0, 6e-12);
  std::normal_distribution<Real> thermal(0.0, thermal_velocity_stddev(350.0, 6.63e-26));
  const Real cx = 0.5 * lat.a1[0];
  const Real cy = 0.5 * lat.a2[1];
  const Real cz = 0.5 * lat.a3[2];
  const Real radius = std::min({lat.a1[0], lat.a2[1], lat.a3[2]}) * 0.35;
  const int n = 12;
  const Real spacing = lat.a1[0] / static_cast<Real>(n);
  for (int ix = 0; ix < n; ++ix) {
    for (int iy = 0; iy < n; ++iy) {
      for (int iz = 0; iz < n; ++iz) {
        const Real px = (static_cast<Real>(ix) + 0.5) * spacing;
        const Real py = (static_cast<Real>(iy) + 0.5) * spacing;
        const Real pz = (static_cast<Real>(iz) + 0.5) * spacing;
        const Real dx = px - cx;
        const Real dy = py - cy;
        const Real dz = pz - cz;
        if ((dx * dx + dy * dy + dz * dz) > radius * radius) continue;

        Particle p;
        p.mass = 6.63e-26;
        p.pos[0] = px + jitter(rng);
        p.pos[1] = py + jitter(rng);
        p.pos[2] = pz + jitter(rng);
        p.vel[0] = thermal(rng);
        p.vel[1] = thermal(rng);
        p.vel[2] = thermal(rng);
        ps.add_particle(p);
      }
    }
  }
  sim.set_potential(std::make_shared<LennardJones>(1.654e-21, 3.405e-10, 1.1e-9));
  sim.set_thermostat(std::make_shared<VelocityRescaleThermostat>(350.0, 8e-13));
}

void populate_thermal_shock(Simulation& sim, const Lattice& lat) {
  sim.set_lattice(lat);

  auto& ps = sim.system();
  ps.clear();
  ps.reserve(2000);

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<Real> uni(0.0, 1.0);
  std::normal_distribution<Real> thermal(0.0, thermal_velocity_stddev(650.0, 6.63e-26));
  const Real cx_left = 0.32 * lat.a1[0];
  const Real cx_right = 0.68 * lat.a1[0];
  const Real cy = 0.5 * lat.a2[1];
  const Real cz = 0.5 * lat.a3[2];
  const Real radius = std::min({lat.a1[0], lat.a2[1], lat.a3[2]}) * 0.25;
  const int n_per_cluster = 700;
  const Real min_dist2 = 8.5e-20;  // (0.29 nm)^2, avoids LJ singular overlaps.
  std::vector<std::array<Real, 3>> placed_positions;
  placed_positions.reserve(static_cast<std::size_t>(2 * n_per_cluster + 160));

  auto can_place = [&](Real px, Real py, Real pz) {
    const Real candidate[3] = {px, py, pz};
    for (const auto& pos : placed_positions) {
      const Real existing[3] = {pos[0], pos[1], pos[2]};
      Real dr[3];
      // Respect periodic boundaries when checking minimum separation.
      const_cast<Lattice&>(lat).min_image_displacement(existing, candidate, dr);
      const Real dx = dr[0];
      const Real dy = dr[1];
      const Real dz = dr[2];
      if (dx * dx + dy * dy + dz * dz < min_dist2) return false;
    }
    return true;
  };

  auto add_particle = [&](Real px, Real py, Real pz, Real drift) {
    Particle p;
    p.mass = 6.63e-26;
    p.pos[0] = px;
    p.pos[1] = py;
    p.pos[2] = pz;
    p.vel[0] = drift + thermal(rng);
    p.vel[1] = thermal(rng);
    p.vel[2] = thermal(rng);
    ps.add_particle(p);
    placed_positions.push_back({px, py, pz});
  };

  auto sample_in_sphere = [&](Real cx, Real drift) {
    int accepted = 0;
    int attempts = 0;
    const int max_attempts = n_per_cluster * 120;
    while (accepted < n_per_cluster && attempts < max_attempts) {
      ++attempts;
      Real rx = 0.0, ry = 0.0, rz = 0.0;
      do {
        rx = (uni(rng) * 2.0 - 1.0) * radius;
        ry = (uni(rng) * 2.0 - 1.0) * radius;
        rz = (uni(rng) * 2.0 - 1.0) * radius;
      } while (rx * rx + ry * ry + rz * rz > radius * radius);
      const Real px = cx + rx;
      const Real py = cy + ry;
      const Real pz = cz + rz;
      if (!can_place(px, py, pz)) continue;
      add_particle(px, py, pz, drift);
      ++accepted;
    }
  };

  sample_in_sphere(cx_left, 180.0);
  sample_in_sphere(cx_right, -180.0);

  int gas_added = 0;
  int gas_attempts = 0;
  while (gas_added < 160 && gas_attempts < 160 * 120) {
    ++gas_attempts;
    const Real px = uni(rng) * lat.a1[0];
    const Real py = uni(rng) * lat.a2[1];
    const Real pz = uni(rng) * lat.a3[2];
    if (!can_place(px, py, pz)) continue;
    add_particle(px, py, pz, 0.0);
    ++gas_added;
  }

  sim.set_potential(std::make_shared<LennardJones>(1.654e-21, 3.405e-10, 1.1e-9));
  sim.set_thermostat(std::make_shared<VelocityRescaleThermostat>(650.0, 8e-13));
}

// ---------------------------------------------------------------------------
// Heat Diffusion: Hot Center (Copper)
//
// A Gaussian hot spot (1200 K) at the center of a 10 cm × 10 cm copper plate,
// surrounded by cold boundaries (300 K).  Demonstrates the heat equation:
// bright center diffuses outward through an inferno-style colormap.
//
// Physics:  ∂T/∂t = α ∇²T,  α_Cu ≈ 1.11 × 10⁻⁴ m²/s.
// Grid:     80 × 80 cells,  Δx = 1.25 mm.
// Stability: Δt ≈ 85 % of limit → safe explicit Euler.
// ---------------------------------------------------------------------------
HeatDiffusion2DParams make_heat_hot_center_params() {
    HeatDiffusion2DParams p;
    p.alpha = 1.11e-4;            // copper thermal diffusivity [m²/s]
    p.dx = 0.1 / 80.0;            // 10 cm domain / 80 cells
    p.nx = 80;
    p.ny = 80;
    p.T_boundary = 300.0;         // room temperature [K]
    p.T_hot = 1200.0;             // peak hot-spot temperature [K]
    p.ic = HeatIC2D::HotCenter;
    p.hot_radius_frac = 0.10;     // Gaussian σ as fraction of domain width
    p.end_time = 0.0;             // run continuously
    p.max_steps = 10000000;
    // dt = 85 % of 2D stability limit
    p.dt = 0.85 * p.stability_limit();
    return p;
}

// ---------------------------------------------------------------------------
// Heat Diffusion: Quenching (Steel)
//
// A 5 cm × 5 cm steel cross-section uniformly at 1200 K is suddenly plunged
// into cold water (300 K Dirichlet boundary).  The cold front advances inward
// from all four edges — a classic industrial heat-treatment scenario.
//
// Physics:  ∂T/∂t = α ∇²T,  α_steel ≈ 1.172 × 10⁻⁵ m²/s.
// Grid:     80 × 80 cells,  Δx = 0.625 mm.
// ---------------------------------------------------------------------------
HeatDiffusion2DParams make_heat_quench_params() {
    HeatDiffusion2DParams p;
    p.alpha = 1.172e-5;           // carbon-steel thermal diffusivity [m²/s]
    p.dx = 0.05 / 80.0;           // 5 cm domain / 80 cells
    p.nx = 80;
    p.ny = 80;
    p.T_boundary = 300.0;         // quenching medium [K]
    p.T_hot = 1200.0;             // initial uniform steel temperature [K]
    p.ic = HeatIC2D::UniformHot;
    p.hot_radius_frac = 0.1;      // unused for UniformHot, but valid
    p.end_time = 0.0;
    p.max_steps = 10000000;
    p.dt = 0.85 * p.stability_limit();
    return p;
}

}  // namespace

struct MainWindow::Impl {
  QTabWidget* tabs{nullptr};
  SimulationTab* sim_tab{nullptr};
  LatticeTab* lattice_tab{nullptr};
  View3DTab* view3d_tab{nullptr};
  QStatusBar* status{nullptr};
  std::unique_ptr<Simulation> simulation;  ///< Active simulation instance
  QTimer sim_timer;                        ///< Timer for non-blocking simulation steps
  QElapsedTimer sim_elapsed;               ///< Track wall-clock time for UI updates
  static constexpr int SIM_TIMER_INTERVAL_MS = 16;  ///< ~60 FPS timer interval
  static constexpr int SIM_BATCH_BUDGET_MS = 12;    ///< Wall-time budget per batch (ms), leaves headroom for rendering
  static constexpr int SIM_MAX_STEPS_PER_TICK_MD = 96;
  static constexpr int SIM_MAX_STEPS_PER_TICK_HEAT = 24;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), impl_(std::make_unique<Impl>()) {
  setWindowTitle(tr("MATSIMU — Material Science Simulator"));
  setMinimumSize(800, 600);
  resize(1024, 720);
  setStyleSheet(
      "QMainWindow{background:#1b1f24;color:#e8edf2;}"
      "QMenuBar{background:#20262d;color:#e8edf2;border-bottom:1px solid #2f3944;}"
      "QMenuBar::item:selected{background:#2b3440;}"
      "QMenu{background:#20262d;color:#e8edf2;border:1px solid #384657;}"
      "QMenu::item:selected{background:#2f89fc;color:white;}"
      "QStatusBar{background:#20262d;color:#c9d5e1;border-top:1px solid #2f3944;}"
      "QTabWidget::pane{border:1px solid #2f3944;top:-1px;background:#1f252c;}"
      "QTabBar::tab{background:#252d36;color:#cfd9e3;padding:10px 16px;margin-right:4px;border-top-left-radius:6px;border-top-right-radius:6px;}"
      "QTabBar::tab:selected{background:#2f89fc;color:#ffffff;font-weight:600;}"
      "QTabBar::tab:hover:!selected{background:#314050;}"
      "QGroupBox{border:1px solid #364352;border-radius:8px;margin-top:14px;padding:14px;background:#222a33;color:#f0f4f8;font-weight:600;}"
      "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 6px;}"
      "QLabel{color:#e3eaf2;font-size:13px;}"
      "QDoubleSpinBox{background:#131920;color:#f4f8fc;border:1px solid #4a5d72;border-radius:6px;padding:6px 8px;selection-background-color:#2f89fc;}"
      "QDoubleSpinBox:hover{border-color:#6e8bad;}"
      "QDoubleSpinBox:focus{border:1px solid #2f89fc;background:#10161c;}"
      "QPushButton{background:#2c3744;color:#eaf2fb;border:1px solid #4a5d72;border-radius:8px;padding:8px 14px;font-weight:600;}"
      "QPushButton:hover{background:#334253;}"
      "QPushButton:disabled{background:#212932;color:#7f8b97;border-color:#3a4552;}"
      "QPushButton#runButton{background:#2f89fc;border-color:#5da7ff;color:white;font-weight:700;}"
      "QPushButton#runButton:hover{background:#4a9cff;}"
      "QPushButton#stopButton{background:#5a2f37;border-color:#8f4753;color:#ffe8eb;}"
      "QPushButton#stopButton:hover{background:#75424c;}"
      "QPushButton#resetButton{background:#2c3744;border-color:#4a5d72;color:#dbe8f4;}"
  );
  
  // Setup simulation timer
  impl_->sim_timer.setInterval(Impl::SIM_TIMER_INTERVAL_MS);
  impl_->sim_timer.setSingleShot(false);
  connect(&impl_->sim_timer, &QTimer::timeout, this, &MainWindow::on_simulation_timer);
  
  setup_menu_bar();
  setup_central();
  setup_status_bar();
  update_status(tr("Ready."));
}

void MainWindow::on_simulation_timer() {
  if (!impl_->simulation) {
    finish_simulation();
    return;
  }
  
  // Run simulation steps within a wall-time budget.
  // Adaptive: fast systems get many steps, heavy systems fewer — always responsive.
  QElapsedTimer batch;
  batch.start();
  int batch_steps = 0;
  const int max_steps_per_tick =
      (impl_->simulation->mode() == SimMode::HeatDiffusion2D)
          ? Impl::SIM_MAX_STEPS_PER_TICK_HEAT
          : Impl::SIM_MAX_STEPS_PER_TICK_MD;
  
  do {
    if (!impl_->simulation->step()) {
      finish_simulation();
      return;
    }
    ++batch_steps;
    if (batch_steps >= max_steps_per_tick) break;
    // Check budget every 10 steps to avoid timer-query overhead
    if (batch_steps % 10 == 0 && batch.hasExpired(Impl::SIM_BATCH_BUDGET_MS)) break;
  } while (!batch.hasExpired(Impl::SIM_BATCH_BUDGET_MS));
  
  // Update UI every tick so users always see a stable, continuous state evolution.
  const Real t = impl_->simulation->time();
  impl_->sim_tab->set_time(t);

  if (impl_->simulation->mode() == SimMode::HeatDiffusion2D) {
    const auto* model = impl_->simulation->heat_2d_model();
    if (model) {
      const auto& T = model->temperature();
      std::vector<Real> T_copy(T.begin(), T.end());
      impl_->view3d_tab->set_temperature_field(
          T_copy, model->nx(), model->ny(),
          model->T_cold(), model->T_hot());
    }
  } else {
    impl_->view3d_tab->set_particles(impl_->simulation->system());
  }

  impl_->view3d_tab->set_simulation_state(true, t, impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());
  impl_->sim_elapsed.restart();
}
 
void MainWindow::finish_simulation() {
  impl_->sim_timer.stop();
  
  if (!impl_->simulation) {
    impl_->sim_tab->set_running(false);
    impl_->lattice_tab->set_editing_enabled(true);
    impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);
    return;
  }
  
  Real final_time = impl_->simulation->time();
  const std::string& err = impl_->simulation->error_message();

  impl_->sim_tab->set_time(final_time);
  impl_->sim_tab->set_running(false);

  if (impl_->simulation->mode() == SimMode::HeatDiffusion2D) {
    const auto* model = impl_->simulation->heat_2d_model();
    if (model) {
      const auto& T = model->temperature();
      std::vector<Real> T_copy(T.begin(), T.end());
      impl_->view3d_tab->set_temperature_field(
          T_copy, model->nx(), model->ny(),
          model->T_cold(), model->T_hot());
    }
  } else {
    impl_->view3d_tab->set_particles(impl_->simulation->system());
  }

  impl_->view3d_tab->set_simulation_state(false, final_time,
                                          impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());

  impl_->lattice_tab->set_editing_enabled(true);

  if (!err.empty()) {
    update_status(tr("Error: %1").arg(QString::fromStdString(err)));
  } else {
    update_status(tr("Run finished at t = %1 s (steps: %2)")
                  .arg(final_time, 0, 'g', 4)
                  .arg(impl_->simulation->step_count()));
  }

  // Keep the final simulation state alive so data persists in memory and
  // remains inspectable until the user explicitly presses Stop or Reset.
}

MainWindow::~MainWindow() = default;

void MainWindow::setup_menu_bar() {
  auto* file = menuBar()->addMenu(tr("&File"));
  auto* act_quit = file->addAction(tr("&Quit"));
  act_quit->setShortcut(QKeySequence::Quit);
  connect(act_quit, &QAction::triggered, this, &QMainWindow::close);

  auto* sim = menuBar()->addMenu(tr("&Simulation"));
  auto* act_run = sim->addAction(tr("&Run"));
  act_run->setShortcut(Qt::Key_F5);
  connect(act_run, &QAction::triggered, this, &MainWindow::on_run);
  auto* act_stop = sim->addAction(tr("S&top"));
  act_stop->setShortcut(Qt::Key_F6);
  connect(act_stop, &QAction::triggered, this, &MainWindow::on_stop);
  auto* act_reset = sim->addAction(tr("&Reset"));
  connect(act_reset, &QAction::triggered, this, &MainWindow::on_reset);

  auto* help = menuBar()->addMenu(tr("&Help"));
  auto* act_about = help->addAction(tr("&About MATSIMU"));
  connect(act_about, &QAction::triggered, this, &MainWindow::on_about);
}

void MainWindow::setup_central() {
  impl_->tabs = new QTabWidget(this);
  impl_->sim_tab = new SimulationTab(this);
  impl_->lattice_tab = new LatticeTab(this);
  impl_->view3d_tab = new View3DTab(this);

  impl_->tabs->addTab(impl_->sim_tab, tr("Simulation"));
  impl_->tabs->addTab(impl_->lattice_tab, tr("Lattice"));
  impl_->tabs->addTab(impl_->view3d_tab, tr("3D View"));

  connect(impl_->lattice_tab, &LatticeTab::lattice_changed,
          impl_->view3d_tab, &View3DTab::set_lattice);
  impl_->view3d_tab->set_lattice(impl_->lattice_tab->lattice());
  impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);

  connect(impl_->sim_tab, &SimulationTab::status_message, this, &MainWindow::update_status);
  connect(impl_->sim_tab, &SimulationTab::run_requested, this, &MainWindow::on_run);
  connect(impl_->sim_tab, &SimulationTab::example_requested, this, &MainWindow::on_run_example);
  connect(impl_->sim_tab, &SimulationTab::example_selection_changed, this, &MainWindow::update_ui_for_example);
  connect(impl_->sim_tab, &SimulationTab::stop_requested, this, &MainWindow::on_stop);
  connect(impl_->sim_tab, &SimulationTab::reset_requested, this, &MainWindow::on_reset);

  setCentralWidget(impl_->tabs);
}

void MainWindow::setup_status_bar() {
  impl_->status = statusBar();
  impl_->status->setSizeGripEnabled(true);
}

void MainWindow::on_run() {
  if (impl_->simulation && impl_->sim_timer.isActive()) {
    update_status(tr("Simulation already running. Press Stop before starting a new run."));
    return;
  }

  const QString example_id = impl_->sim_tab->selected_example_id();

  // -----------------------------------------------------------------------
  // Heat diffusion examples (2D continuum)
  // -----------------------------------------------------------------------
  if (example_id == "heat_hot_center" || example_id == "heat_quench") {
    // For heat diffusion, we currently use the hardcoded example params 
    // because the UI doesn't expose heat-specific settings (alpha, nx, ny).
    auto heat_params = (example_id == "heat_hot_center")
                                 ? make_heat_hot_center_params()
                                 : make_heat_quench_params();
    
    // Override from UI only when valid (dt must stay within 2D stability limit)
    const Real limit = heat_params.stability_limit();
    auto ui_params = impl_->sim_tab->params();
    if (std::isfinite(limit) && limit > 0 && ui_params.dt > 0 && ui_params.dt <= limit)
      heat_params.dt = ui_params.dt;
    if (ui_params.end_time >= 0) heat_params.end_time = ui_params.end_time;

    impl_->simulation = std::make_unique<matsimu::Simulation>(heat_params);
    if (!impl_->simulation->is_valid()) {
      update_status(tr("Error: %1").arg(QString::fromStdString(impl_->simulation->error_message())));
      impl_->simulation.reset();
      return;
    }

    const auto* model = impl_->simulation->heat_2d_model();
    if (model) {
      const auto& T = model->temperature();
      std::vector<Real> T_copy(T.begin(), T.end());
      impl_->view3d_tab->set_temperature_field(
          T_copy, model->nx(), model->ny(),
          model->T_cold(), model->T_hot());
    }

    impl_->sim_tab->set_running(true);
    impl_->sim_tab->set_time(0.0);
    impl_->lattice_tab->set_editing_enabled(false);
    impl_->view3d_tab->set_simulation_state(true, 0.0, heat_params.end_time, 0);
    impl_->sim_elapsed.start();

    update_status(tr("Running %1 simulation.").arg(example_id == "heat_hot_center" ? "Heat Hot Center" : "Heat Quenching"));
    if (impl_->tabs) impl_->tabs->setCurrentWidget(impl_->view3d_tab);
    impl_->sim_timer.start();
    return;
  }

  // -----------------------------------------------------------------------
  // Particle-based examples (molecular dynamics)
  // -----------------------------------------------------------------------
  matsimu::SimulationParams params = impl_->sim_tab->params();
  params.temperature = (example_id == "thermal_shock") ? 650.0 : 350.0;
  params.cutoff = 1.1e-9;
  params.use_neighbor_list = true;
  params.neighbor_skin = 2.5e-10;
  
  if (params.end_time <= 0.0 || !std::isfinite(params.end_time)) {
    params.end_time = 0.0;
    params.max_steps = std::numeric_limits<std::size_t>::max();
  }
  
  auto validation_error = params.validate();
  if (validation_error) {
    update_status(tr("Invalid simulation parameters: %1").arg(QString::fromStdString(*validation_error)));
    return;
  }
  
  impl_->simulation = std::make_unique<matsimu::Simulation>(params);
  if (!impl_->simulation->is_valid()) {
    update_status(tr("Error: Failed to initialize simulation"));
    impl_->simulation.reset();
    return;
  }

  Lattice lat = impl_->lattice_tab->lattice();
  if (example_id == "thermal_shock") {
    populate_thermal_shock(*impl_->simulation, lat);
    update_status(tr("Running Thermal Shock (colliding clusters)."));
  } else {
    // Default or Argon
    populate_argon_crystal(*impl_->simulation, lat);
    update_status(tr("Running Argon Crystal relaxation."));
  }
  
  impl_->simulation->initialize();
  impl_->lattice_tab->set_lattice(lat);
  impl_->view3d_tab->set_lattice(lat);
  impl_->view3d_tab->set_particles(impl_->simulation->system());

  impl_->sim_tab->set_running(true);
  impl_->sim_tab->set_time(impl_->simulation->time());
  impl_->lattice_tab->set_editing_enabled(false);
  impl_->view3d_tab->set_simulation_state(true, impl_->simulation->time(),
                                          impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());
  impl_->sim_elapsed.start();

  if (impl_->tabs) impl_->tabs->setCurrentWidget(impl_->view3d_tab);
  impl_->sim_timer.start();
}

void MainWindow::update_ui_for_example(const QString& example_id) {
  if (example_id.isEmpty()) return;

  if (example_id == "argon_crystal") {
    SimulationParams p;
    p.dx = 1e-9; p.dt = 1e-15; p.end_time = 0.0;
    impl_->sim_tab->set_params(p);
    Lattice lat;
    lat.a1[0] = 8e-9; lat.a2[1] = 8e-9; lat.a3[2] = 8e-9;
    impl_->lattice_tab->set_lattice(lat);
  } else if (example_id == "thermal_shock") {
    SimulationParams p;
    p.dx = 1e-9; p.dt = 1e-15; p.end_time = 0.0;
    impl_->sim_tab->set_params(p);
    Lattice lat;
    lat.a1[0] = 10e-9; lat.a2[1] = 10e-9; lat.a3[2] = 10e-9;
    impl_->lattice_tab->set_lattice(lat);
  } else if (example_id == "heat_hot_center") {
    auto p = make_heat_hot_center_params();
    SimulationParams sp; sp.dx = p.dx; sp.dt = p.dt; sp.end_time = p.end_time;
    impl_->sim_tab->set_params(sp);
  } else if (example_id == "heat_quench") {
    auto p = make_heat_quench_params();
    SimulationParams sp; sp.dx = p.dx; sp.dt = p.dt; sp.end_time = p.end_time;
    impl_->sim_tab->set_params(sp);
  }
}

void MainWindow::on_run_example(const QString& example_id) {
  if (example_id.isEmpty()) {
    update_status(tr("No example selected. Choose an example from the list and try again."));
    return;
  }
  on_stop();
  update_ui_for_example(example_id);
  on_run();
}

void MainWindow::on_stop() {
  const bool was_active = (impl_->simulation && impl_->sim_timer.isActive());
  impl_->sim_timer.stop();
  
  if (impl_->simulation) {
    const Real t = impl_->simulation->time();
    impl_->sim_tab->set_time(t);

    if (impl_->simulation->mode() == SimMode::HeatDiffusion2D) {
      const auto* model = impl_->simulation->heat_2d_model();
      if (model) {
        const auto& T = model->temperature();
        std::vector<Real> T_copy(T.begin(), T.end());
        impl_->view3d_tab->set_temperature_field(
            T_copy, model->nx(), model->ny(),
            model->T_cold(), model->T_hot());
      }
    } else {
      impl_->view3d_tab->set_particles(impl_->simulation->system());
    }

    impl_->view3d_tab->set_simulation_state(false, t,
                                            impl_->simulation->params().end_time,
                                            impl_->simulation->step_count());
    impl_->simulation.reset();
  } else {
    impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);
  }

  impl_->sim_tab->set_running(false);
  impl_->lattice_tab->set_editing_enabled(true);
  if (was_active) {
    update_status(tr("Stopped."));
  }
}

void MainWindow::on_reset() {
  on_stop();
  
  impl_->sim_tab->set_time(0.0);
  impl_->view3d_tab->clear_particles();
  impl_->view3d_tab->clear_temperature_field();
  impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);
  update_status(tr("Reset."));
}

void MainWindow::on_about() {
  QMessageBox::about(this, tr("About MATSIMU"),
                     tr("<h3>MATSIMU</h3>"
                        "<p>Material Science Simulator — Linux-native, educational.</p>"
                        "<p>Dual audience: domain experts and learners.</p>"));
}

void MainWindow::update_status(const QString& text) {
  if (impl_->status)
    impl_->status->showMessage(text, 5000);
}

}  // namespace matsimu
