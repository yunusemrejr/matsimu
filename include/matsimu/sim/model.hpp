#pragma once

#include <matsimu/core/types.hpp>
#include <cstddef>
#include <string>

namespace matsimu {

/**
 * Interface for physics models used by Simulation.
 * Simulation orchestrates stepping and termination; model implements the math.
 */
class ISimModel {
public:
  virtual ~ISimModel() = default;

  /// Advance one step. Returns false when step failed or simulation finished.
  virtual bool step() = 0;
  /// True when simulation has reached end condition (time, steps, or error).
  virtual bool finished() const = 0;
  /// Current simulation time [s].
  virtual Real time() const = 0;
  /// Number of steps taken.
  virtual std::size_t step_count() const = 0;
  /// Error message if invalid/failed; empty otherwise. Never null.
  virtual const std::string& error_message() const = 0;
  /// Whether the model is valid and can step.
  virtual bool is_valid() const = 0;
};

}  // namespace matsimu
