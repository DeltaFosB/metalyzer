#pragma once

#include <map>
#include <metalyzer/lexer/DFA.hpp>
#include <set>
#include <vector>

namespace metalyzer {

class Minimizer {
public:
  DFA minimize(const DFA &dfa, DFAContext &ctx);

private:
  std::vector<std::vector<DFAState *>> initPartitions(const DFA &dfa);

  std::vector<std::vector<DFAState *>>
  refinePartitions(const std::vector<std::vector<DFAState *>> &partitions,
                   const std::set<char> &alphabet);

  bool splitSet(const std::vector<DFAState *> &currentGroup,
                const std::set<char> &alphabet,
                const std::unordered_map<int, int> &stateToGroup,
                std::vector<std::vector<DFAState *>> &outputSubGroups);

  DFA makeDFA(const DFA &originalDFA,
              const std::vector<std::vector<DFAState *>> &partitions,
              DFAContext &ctx);

  std::set<char> getAlphabet(const DFA &dfa);
};

} // namespace metalyzer
