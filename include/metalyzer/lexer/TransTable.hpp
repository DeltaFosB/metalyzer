#pragma once

#include <vector>

namespace metalyzer {
struct TransTable {
  std::vector<std::vector<int>> table;
  std::vector<bool> isAccepting;
  int startStateId;
};
} // namespace metalyzer
