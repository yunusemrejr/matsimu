#include <matsimu/ui/main_window.hpp>
#include <matsimu/ui/simulation_tab.hpp>
#include <matsimu/ui/lattice_tab.hpp>
#include <matsimu/ui/view_3d_tab.hpp>
#include <matsimu/sim/simulation.hpp>
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

namespace matsimu {

struct MainWindow::Impl {
  QTabWidget* tabs{nullptr};
  SimulationTab* sim_tab{nullptr};
  LatticeTab* lattice_tab{nullptr};
  View3DTab* view3d_tab{nullptr};
  QStatusBar* status{nullptr};
  std::unique_ptr<Simulation> simulation;  ///< Active simulation instance
  QTimer sim_timer;                        ///< Timer for non-blocking simulation steps
  QElapsedTimer sim_elapsed;               ///< Track wall-clock time for UI updates
  static constexpr int SIM_STEPS_PER_BATCH = 100;  ///< Steps per timer batch for UI responsiveness
  static constexpr int SIM_TIMER_INTERVAL_MS = 16; ///< ~60 FPS timer interval
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), impl_(std::make_unique<Impl>()) {
  setWindowTitle(tr("MATSIMU — Material Science Simulator"));
  setMinimumSize(800, 600);
  resize(1024, 720);
  
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
  
  // Run a batch of steps to balance performance with UI responsiveness
  for (int i = 0; i < Impl::SIM_STEPS_PER_BATCH; ++i) {
    if (!impl_->simulation->step()) {
      finish_simulation();
      return;
    }
  }
  
  // Update UI with current time (throttle updates to avoid flicker)
  if (impl_->sim_elapsed.hasExpired(50)) {  // Update every 50ms
    impl_->sim_tab->set_time(impl_->simulation->time());
    impl_->sim_elapsed.restart();
  }
}

void MainWindow::finish_simulation() {
  impl_->sim_timer.stop();
  
  if (!impl_->simulation) {
    impl_->sim_tab->set_running(false);
    return;
  }
  
  Real final_time = impl_->simulation->time();
  const std::string& err = impl_->simulation->error_message();

  impl_->sim_tab->set_time(final_time);
  impl_->sim_tab->set_running(false);

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

  connect(impl_->sim_tab, &SimulationTab::status_message, this, &MainWindow::update_status);
  connect(impl_->sim_tab, &SimulationTab::run_requested, this, &MainWindow::on_run);
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
  
  // Get and validate parameters
  matsimu::SimulationParams params = impl_->sim_tab->params();
  
  // Auto-set end_time if not specified
  if (params.end_time <= 0.0 || !std::isfinite(params.end_time)) {
    params.end_time = 2.0 * params.dt;
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
  
  impl_->sim_tab->set_running(true);
  impl_->sim_tab->set_time(impl_->simulation->time());
  impl_->sim_elapsed.start();
  
  update_status(tr("Running simulation (target t = %1 s, max steps = %2)")
                .arg(params.end_time, 0, 'g', 4)
                .arg(params.max_steps));
  
  // Start the timer-based simulation loop (non-blocking)
  impl_->sim_timer.start();
}

void MainWindow::on_stop() {
  // Stop the timer
  impl_->sim_timer.stop();
  
  // Clean up simulation if running
  if (impl_->simulation) {
    impl_->sim_tab->set_time(impl_->simulation->time());
    impl_->simulation.reset();
  }
  
  impl_->sim_tab->set_running(false);
  update_status(tr("Stopped."));
}

void MainWindow::on_reset() {
  // Stop any running simulation first
  on_stop();
  
  impl_->sim_tab->set_time(0);
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
