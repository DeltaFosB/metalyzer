#pragma once
#include <string>
#include <vector>

namespace metalyzer {
namespace frontend {

struct LexerRule {
  std::string regex;      // e.g. "[0-9]+"
  std::string actionCode; // e.g. "{ return 1; }"
  int priority;           // 0, 1, 2... (Lower number = Higher priority)
};

struct LexerSpec {
  std::string headerCode; // Content from %{ ... %}
  std::vector<LexerRule> rules;
  std::string userCode; // Content after the second %%
};

} // namespace frontend
} // namespace metalyzer
