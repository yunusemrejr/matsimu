#pragma once

#include <QWidget>
#include <matsimu/lattice/lattice.hpp>

namespace matsimu {

class View3D;

/**
 * Tab that hosts the 3D view and optional overlay controls.
 */
class View3DTab : public QWidget {
  Q_OBJECT
 public:
  explicit View3DTab(QWidget* parent = nullptr);
  ~View3DTab() override;

  void set_lattice(const Lattice& lat);

 private:
  View3D* view_{nullptr};
};

}  // namespace matsimu
