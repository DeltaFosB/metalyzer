#pragma once

#include <vector>

namespace metalyzer {
struct TransTable {
  // A flattened 2D array [StateID][ASCII]
  std::vector<std::vector<int>> table;
  std::vector<bool> isAccepting;
  int startStateId;
};
} // namespace metalyzer
