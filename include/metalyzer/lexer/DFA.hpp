#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace metalyzer {
namespace lexer {

struct DFAState {
  int id;
  std::unordered_map<char, DFAState *> transitions;
  int acceptRuleId = -1;
  DFAState(int id) : id(id) {}
};

class DFAContext {
  std::vector<std::unique_ptr<DFAState>> registry;
  int nextId = 0;

public:
  DFAState *createState() {
    auto state = std::make_unique<DFAState>(nextId++);
    DFAState *ptr = state.get();
    registry.push_back(std::move(state));
    return ptr;
  }
};

struct DFA {
  DFAState *start = nullptr;
  std::vector<DFAState *> allStates;
};

} // namespace lexer
} // namespace metalyzer
