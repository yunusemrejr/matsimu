#pragma once

#include <QWidget>
#include <matsimu/lattice/lattice.hpp>

class QDoubleSpinBox;
class QGroupBox;
class QLabel;

namespace matsimu {

/**
 * Lattice tab: edit basis vectors a1, a2, a3 (SI, m) and show volume.
 * Read/write a Lattice copy; notifies when lattice changes for 3D view.
 */
class LatticeTab : public QWidget {
  Q_OBJECT
 public:
  explicit LatticeTab(QWidget* parent = nullptr);
  ~LatticeTab() override;

  Lattice lattice() const;
  void set_lattice(const Lattice& lat);

 signals:
  void lattice_changed(const matsimu::Lattice& lat);

 private:
  void build_ui();
  void sync_from_ui();
  void update_volume_label();
  void assign_spinbox_values_to_vector(Real target_array[3], QDoubleSpinBox* spinboxes[3]);

  QGroupBox* basis_group_{nullptr};
  QDoubleSpinBox* a1_[3]{nullptr, nullptr, nullptr};
  QDoubleSpinBox* a2_[3]{nullptr, nullptr, nullptr};
  QDoubleSpinBox* a3_[3]{nullptr, nullptr, nullptr};
  QLabel* label_volume_{nullptr};
  Lattice lattice_;
};

}  // namespace matsimu
