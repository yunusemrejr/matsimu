#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/sim/simulation.hpp>
#include <stdexcept>
#include <string>

namespace matsimu {

/**
 * Result of loading config: either params or an error message.
 * Use for explicit error handling without exceptions.
 */
struct ConfigResult {
  bool ok{false};
  SimulationParams params;
  std::string error;

  static ConfigResult success(SimulationParams p) {
    ConfigResult r;
    r.ok = true;
    r.params = std::move(p);
    return r;
  }
  static ConfigResult failure(std::string msg) {
    ConfigResult r;
    r.error = std::move(msg);
    return r;
  }
};

/**
 * Load simulation parameters from a config file path.
 *
 * Contract:
 * - Empty path: returns default params (ConfigResult.ok = true, default SimulationParams).
 * - Non-empty path: reads file; on success returns parsed params (SI); on file-not-found
 *   or parse error returns ok = false and a non-empty error message (no silent defaults).
 *
 * File format: one key=value per line; '#' comment; keys: dt, dx, end_time, max_steps,
 * temperature, cutoff, neighbor_skin, use_neighbor_list. All numeric values in SI.
 * Conversions only at this I/O boundary; core simulation uses SI.
 */
ConfigResult load_config(const std::string& path);

/**
 * Load config; throws std::runtime_error on invalid non-empty path.
 * Empty path returns default params.
 */
SimulationParams load_config_or_throw(const std::string& path);

}  // namespace matsimu
