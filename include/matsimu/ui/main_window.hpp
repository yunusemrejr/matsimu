#pragma once

#include <memory>
#include <QMainWindow>
#include <QTimer>

class QTabWidget;
class QStatusBar;
class QMenuBar;
class QWidget;

namespace matsimu {

class Simulation;

/**
 * Main application window: menu bar, tabbed central area, status bar.
 * Owns all UI controllers and views; does not modify simulation/lattice logic.
 * Uses timer-based simulation stepping to keep UI responsive.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

 private slots:
  void setup_menu_bar();
  void setup_central();
  void setup_status_bar();
  void on_run();
  void on_stop();
  void on_reset();
  void on_about();
  void update_status(const QString& text);
  void on_simulation_timer();  ///< Timer callback for non-blocking simulation steps
  void finish_simulation();    ///< Clean up after simulation completes

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace matsimu
