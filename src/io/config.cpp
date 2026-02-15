#include <matsimu/io/config.hpp>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace matsimu {

namespace {

std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

bool parse_double(const std::string& value, Real& out) {
  std::istringstream is(value);
  Real x;
  if (!(is >> x)) return false;
  char c;
  if (is >> c) return false;
  out = x;
  return true;
}

bool parse_size_t(const std::string& value, std::size_t& out) {
  std::istringstream is(value);
  unsigned long x;
  if (!(is >> x)) return false;
  char c;
  if (is >> c) return false;
  out = static_cast<std::size_t>(x);
  return true;
}

bool parse_bool(const std::string& value, bool& out) {
  std::string v = value;
  std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
  if (v == "1" || v == "true" || v == "yes") { out = true; return true; }
  if (v == "0" || v == "false" || v == "no") { out = false; return true; }
  return false;
}

}  // namespace

ConfigResult load_config(const std::string& path) {
  SimulationParams p;
  if (path.empty())
    return ConfigResult::success(p);

  std::ifstream f(path);
  if (!f.is_open())
    return ConfigResult::failure("Cannot open config file: " + path);

  std::string line;
  int line_no = 0;
  while (std::getline(f, line)) {
    ++line_no;
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    std::size_t eq = line.find('=');
    if (eq == std::string::npos)
      return ConfigResult::failure("Invalid line " + std::to_string(line_no) + ": missing '='");
    std::string key = trim(line.substr(0, eq));
    std::string value = trim(line.substr(eq + 1));
    if (key.empty())
      return ConfigResult::failure("Invalid line " + std::to_string(line_no) + ": empty key");

    if (key == "dt") {
      if (!parse_double(value, p.dt))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid dt value");
    } else if (key == "dx") {
      if (!parse_double(value, p.dx))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid dx value");
    } else if (key == "end_time") {
      if (!parse_double(value, p.end_time))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid end_time value");
    } else if (key == "max_steps") {
      if (!parse_size_t(value, p.max_steps))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid max_steps value");
    } else if (key == "temperature") {
      if (!parse_double(value, p.temperature))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid temperature value");
    } else if (key == "cutoff") {
      if (!parse_double(value, p.cutoff))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid cutoff value");
    } else if (key == "neighbor_skin") {
      if (!parse_double(value, p.neighbor_skin))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid neighbor_skin value");
    } else if (key == "use_neighbor_list") {
      if (!parse_bool(value, p.use_neighbor_list))
        return ConfigResult::failure("Line " + std::to_string(line_no) + ": invalid use_neighbor_list value");
    } else {
      return ConfigResult::failure("Line " + std::to_string(line_no) + ": unknown key '" + key + "'");
    }
  }

  if (!f.eof())
    return ConfigResult::failure("Error reading config file");

  auto validation = p.validate();
  if (validation)
    return ConfigResult::failure("Config validation failed: " + *validation);

  return ConfigResult::success(p);
}

SimulationParams load_config_or_throw(const std::string& path) {
  ConfigResult r = load_config(path);
  if (r.ok) return r.params;
  throw std::runtime_error(r.error);
}

}  // namespace matsimu
