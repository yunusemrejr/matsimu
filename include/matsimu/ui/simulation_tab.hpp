#pragma once

#include <QWidget>
#include <matsimu/core/types.hpp>
#include <matsimu/sim/simulation.hpp>

class QPushButton;
class QDoubleSpinBox;
class QLabel;
class QGroupBox;

namespace matsimu {

/**
 * Simulation control tab: parameters (dx, dt, end_time), Run / Stop / Reset.
 * Emits signals or calls back so main window or runner can drive Simulation.
 * Does not own Simulation; only holds params and control state.
 */
class SimulationTab : public QWidget {
  Q_OBJECT
 public:
  explicit SimulationTab(QWidget* parent = nullptr);
  ~SimulationTab() override;

  SimulationParams params() const;
  bool is_running() const { return running_; }

 signals:
  void run_requested();
  void stop_requested();
  void reset_requested();
  void status_message(const QString& msg);

 public slots:
  void set_running(bool running);
  void set_time(Real t);
  void set_params(const SimulationParams& p);

 private:
  void build_ui();
  void on_run_clicked();
  void on_stop_clicked();
  void on_reset_clicked();

  QGroupBox* params_group_{nullptr};
  QDoubleSpinBox* spin_dx_{nullptr};
  QDoubleSpinBox* spin_dt_{nullptr};
  QDoubleSpinBox* spin_end_time_{nullptr};
  QPushButton* btn_run_{nullptr};
  QPushButton* btn_stop_{nullptr};
  QPushButton* btn_reset_{nullptr};
  QLabel* label_time_{nullptr};
  bool running_{false};
};

}  // namespace matsimu
