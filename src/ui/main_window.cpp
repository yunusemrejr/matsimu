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
#include <limits>
#include <random>

namespace matsimu {

namespace {

void populate_argon_crystal(Simulation& sim, Lattice& lat) {
  lat.a1[0] = 8.0e-9; lat.a1[1] = 0.0;    lat.a1[2] = 0.0;
  lat.a2[0] = 0.0;    lat.a2[1] = 8.0e-9; lat.a2[2] = 0.0;
  lat.a3[0] = 0.0;    lat.a3[1] = 0.0;    lat.a3[2] = 8.0e-9;
  sim.set_lattice(lat);

  auto& ps = sim.system();
  ps.clear();
  ps.reserve(1600);

  std::mt19937 rng(42u);
  std::normal_distribution<Real> jitter(0.0, 6e-12);
  std::normal_distribution<Real> thermal(0.0, 110.0);
  const Real cx = 0.5 * lat.a1[0];
  const Real cy = 0.5 * lat.a2[1];
  const Real cz = 0.5 * lat.a3[2];
  const Real radius = 2.7e-9;
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

void populate_thermal_shock(Simulation& sim, Lattice& lat) {
  lat.a1[0] = 10.0e-9; lat.a1[1] = 0.0;     lat.a1[2] = 0.0;
  lat.a2[0] = 0.0;     lat.a2[1] = 10.0e-9; lat.a2[2] = 0.0;
  lat.a3[0] = 0.0;     lat.a3[1] = 0.0;     lat.a3[2] = 10.0e-9;
  sim.set_lattice(lat);

  auto& ps = sim.system();
  ps.clear();
  ps.reserve(2000);

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<Real> uni(0.0, 1.0);
  std::normal_distribution<Real> thermal(0.0, 80.0);
  const Real cx_left = 0.32 * lat.a1[0];
  const Real cx_right = 0.68 * lat.a1[0];
  const Real cy = 0.5 * lat.a2[1];
  const Real cz = 0.5 * lat.a3[2];
  const Real radius = 2.1e-9;
  const int n_per_cluster = 700;

  auto sample_in_sphere = [&](Real cx) {
    for (int i = 0; i < n_per_cluster; ++i) {
      Real rx = 0.0, ry = 0.0, rz = 0.0;
      do {
        rx = (uni(rng) * 2.0 - 1.0) * radius;
        ry = (uni(rng) * 2.0 - 1.0) * radius;
        rz = (uni(rng) * 2.0 - 1.0) * radius;
      } while (rx * rx + ry * ry + rz * rz > radius * radius);

      Particle p;
      p.mass = 6.63e-26;
      p.pos[0] = cx + rx;
      p.pos[1] = cy + ry;
      p.pos[2] = cz + rz;
      const Real drift = (cx < 0.5 * lat.a1[0]) ? 180.0 : -180.0;
      p.vel[0] = drift + thermal(rng);
      p.vel[1] = thermal(rng);
      p.vel[2] = thermal(rng);
      ps.add_particle(p);
    }
  };

  sample_in_sphere(cx_left);
  sample_in_sphere(cx_right);
  for (int i = 0; i < 160; ++i) {
    Particle p;
    p.mass = 6.63e-26;
    p.pos[0] = uni(rng) * lat.a1[0];
    p.pos[1] = uni(rng) * lat.a2[1];
    p.pos[2] = uni(rng) * lat.a3[2];
    p.vel[0] = thermal(rng);
    p.vel[1] = thermal(rng);
    p.vel[2] = thermal(rng);
    ps.add_particle(p);
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
  
  do {
    if (!impl_->simulation->step()) {
      finish_simulation();
      return;
    }
    ++batch_steps;
    // Check budget every 10 steps to avoid timer-query overhead
    if (batch_steps % 10 == 0 && batch.hasExpired(Impl::SIM_BATCH_BUDGET_MS)) break;
  } while (!batch.hasExpired(Impl::SIM_BATCH_BUDGET_MS));
  
  // Update UI with current time (throttle updates to avoid flicker)
  if (impl_->sim_elapsed.hasExpired(50)) {  // Update every 50ms
    const Real t = impl_->simulation->time();
    impl_->sim_tab->set_time(t);
    impl_->view3d_tab->set_particles(impl_->simulation->system());
    impl_->view3d_tab->set_simulation_state(true, t, impl_->simulation->params().end_time,
                                            impl_->simulation->step_count());
    impl_->sim_elapsed.restart();
  }
}

void MainWindow::finish_simulation() {
  impl_->sim_timer.stop();
  
  if (!impl_->simulation) {
    impl_->sim_tab->set_running(false);
    impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);
    return;
  }
  
  Real final_time = impl_->simulation->time();
  const std::string& err = impl_->simulation->error_message();

  impl_->sim_tab->set_time(final_time);
  impl_->sim_tab->set_running(false);
  impl_->view3d_tab->set_particles(impl_->simulation->system());
  impl_->view3d_tab->set_simulation_state(false, final_time,
                                          impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());

  if (!err.empty()) {
    update_status(tr("Error: %1").arg(QString::fromStdString(err)));
  } else {
    update_status(tr("Run finished at t = %1 s (steps: %2)")
                  .arg(final_time, 0, 'g', 4)
                  .arg(impl_->simulation->step_count()));
  }
  
  // Release simulation resources
  impl_->simulation.reset();
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
  connect(impl_->sim_tab, &SimulationTab::stop_requested, this, &MainWindow::on_stop);
  connect(impl_->sim_tab, &SimulationTab::reset_requested, this, &MainWindow::on_reset);

  setCentralWidget(impl_->tabs);
}

void MainWindow::setup_status_bar() {
  impl_->status = statusBar();
  impl_->status->setSizeGripEnabled(true);
}

void MainWindow::on_run() {
  // Stop any existing simulation first
  on_stop();
  
  // Read user-visible parameters from UI, fill in MD physics defaults
  matsimu::SimulationParams params = impl_->sim_tab->params();
  params.temperature = 300.0;
  params.cutoff = 1.1e-9;
  params.use_neighbor_list = true;
  params.neighbor_skin = 2.5e-10;
  
  // end_time <= 0 means continuous run until user presses Stop.
  if (params.end_time <= 0.0 || !std::isfinite(params.end_time)) {
    params.end_time = 0.0;
    params.max_steps = std::numeric_limits<std::size_t>::max();
  }
  
  auto validation_error = params.validate();
  if (validation_error) {
    update_status(tr("Error: Invalid simulation parameters"));
    QMessageBox::warning(this, tr("Invalid Parameters"),
                         tr("Please check parameters: %1").arg(QString::fromStdString(*validation_error)));
    return;
  }
  
  // Create simulation instance
  impl_->simulation = std::make_unique<matsimu::Simulation>(params);
  
  if (!impl_->simulation->is_valid()) {
    update_status(tr("Error: Failed to initialize simulation"));
    impl_->simulation.reset();
    return;
  }

  // Auto-populate with default argon crystal scene.
  // No particle editor in the UI yet, so "Run" always launches a working example.
  Lattice lat;
  populate_argon_crystal(*impl_->simulation, lat);
  impl_->lattice_tab->set_lattice(lat);
  impl_->view3d_tab->set_lattice(lat);
  
  // Initialize: zero COM velocity drift, compute initial forces for Velocity Verlet.
  impl_->simulation->initialize();
  
  impl_->view3d_tab->set_particles(impl_->simulation->system());
  
  impl_->sim_tab->set_running(true);
  impl_->sim_tab->set_time(impl_->simulation->time());
  impl_->view3d_tab->set_simulation_state(true, impl_->simulation->time(),
                                          impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());
  impl_->sim_elapsed.start();
  
  update_status(tr("Running Argon Crystal simulation (press Stop to end)."));
  
  // Switch to 3D View for immediate visual feedback
  if (impl_->tabs) {
    impl_->tabs->setCurrentWidget(impl_->view3d_tab);
  }
  
  // Start the timer-based simulation loop (non-blocking)
  impl_->sim_timer.start();
}

void MainWindow::on_run_example(const QString& example_id) {
  on_stop();

  matsimu::SimulationParams params;
  params.dx = 1e-9;
  params.end_time = 0.0;
  params.max_steps = std::numeric_limits<std::size_t>::max();
  params.dt = 1e-15;
  params.temperature = 500.0;
  params.cutoff = 1.1e-9;
  params.use_neighbor_list = true;
  params.neighbor_skin = 2.5e-10;

  impl_->simulation = std::make_unique<matsimu::Simulation>(params);
  if (!impl_->simulation->is_valid()) {
    update_status(tr("Error: Failed to initialize example simulation"));
    impl_->simulation.reset();
    return;
  }

  Lattice lat = impl_->lattice_tab->lattice();
  if (example_id == "thermal_shock") {
    populate_thermal_shock(*impl_->simulation, lat);
  } else {
    populate_argon_crystal(*impl_->simulation, lat);
  }

  impl_->lattice_tab->set_lattice(lat);
  impl_->view3d_tab->set_lattice(lat);

  // Initialize: zero COM velocity drift, compute initial forces for Velocity Verlet.
  impl_->simulation->initialize();

  impl_->view3d_tab->set_particles(impl_->simulation->system());

  impl_->sim_tab->set_running(true);
  impl_->sim_tab->set_time(impl_->simulation->time());
  impl_->view3d_tab->set_simulation_state(true, impl_->simulation->time(),
                                          impl_->simulation->params().end_time,
                                          impl_->simulation->step_count());
  impl_->sim_elapsed.start();
  update_status(tr("Running one-click example: %1 (press Stop to end).")
                    .arg(example_id == "thermal_shock" ? tr("Thermal Shock") : tr("Argon Crystal")));
  if (impl_->tabs) {
    impl_->tabs->setCurrentWidget(impl_->view3d_tab);
  }
  impl_->sim_timer.start();
}

void MainWindow::on_stop() {
  // Stop the timer
  impl_->sim_timer.stop();
  
  // Clean up simulation if running
  if (impl_->simulation) {
    const Real t = impl_->simulation->time();
    impl_->sim_tab->set_time(t);
    impl_->view3d_tab->set_particles(impl_->simulation->system());
    impl_->view3d_tab->set_simulation_state(false, t,
                                            impl_->simulation->params().end_time,
                                            impl_->simulation->step_count());
    impl_->simulation.reset();
  } else {
    impl_->view3d_tab->set_simulation_state(false, 0.0, 0.0, 0);
  }
  
  impl_->sim_tab->set_running(false);
  update_status(tr("Stopped."));
}

void MainWindow::on_reset() {
  // Stop any running simulation first
  on_stop();
  
  impl_->sim_tab->set_time(0);
  impl_->view3d_tab->clear_particles();
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
