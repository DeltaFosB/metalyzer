#pragma once

#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/NFA.hpp>
#include <set>
#include <vector>

namespace metalyzer {

class PowerSetConstructor {
public:
  explicit PowerSetConstructor(DFAContext &context) : ctx(context) {};
  DFA convert(const NFA &nfa);

private:
  DFAContext &ctx;
  std::unordered_map<int, std::set<NFAState *>> closureCache;

  std::set<NFAState *> computeEpsilonClosure(NFAState *state);
  std::set<NFAState *>
  computeEpsilonClosure(const std::set<NFAState *> &states);
  std::set<char> getAlphabet(const NFA &nfa);
};

} // namespace metalyzer
