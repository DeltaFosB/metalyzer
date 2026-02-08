#pragma once

#include <vector>

namespace metalyzer {
namespace lexer {
struct TransTable {
  std::vector<std::vector<int>> table;
  std::vector<int> acceptRuleIds;
  int startStateId;
};
} // namespace lexer
} // namespace metalyzer
