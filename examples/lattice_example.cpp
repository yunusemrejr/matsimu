// Standalone compile: g++ -I ../include -std=c++17 -o lattice_example lattice_example.cpp ../src/lattice/lattice.cpp
// Or run via main: ./run.sh --example lattice
#include <matsimu/lattice/lattice.hpp>
#include <iostream>

int main() {
  matsimu::Lattice lat;
  std::cout << "Lattice volume: " << lat.volume() << " m³\n";
  return 0;
}
