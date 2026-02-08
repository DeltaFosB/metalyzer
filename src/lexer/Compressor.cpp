#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/TransTable.hpp>
#include <unordered_map>

namespace metalyzer {
namespace lexer {

const int ALPHABET_SIZE = 128;

TransTable Compressor::compress(const DFA &dfa) {

  TransTable result;

  if (dfa.allStates.empty()) {
    result.startStateId = -1;
    return result;
  }

  std::unordered_map<int, int> stateIdToIndex;
  int nextIndex = 0;

  for (DFAState *s : dfa.allStates) {
    stateIdToIndex[s->id] = nextIndex++;

    result.acceptRuleIds.push_back(s->acceptRuleId);
  }

  int numStates = dfa.allStates.size();
  result.table.assign(numStates, std::vector<int>(ALPHABET_SIZE, -1));

  for (DFAState *s : dfa.allStates) {
    int rowIndex = stateIdToIndex[s->id];

    for (auto const &[char_code, targetState] : s->transitions) {
      if (char_code >= 0 && char_code < ALPHABET_SIZE) {
        int colIndex = static_cast<int>(char_code);
        int targetIndex = stateIdToIndex[targetState->id];

        result.table[rowIndex][colIndex] = targetIndex;
      }
    }
  }

  if (dfa.start) {
    result.startStateId = stateIdToIndex[dfa.start->id];
  } else {
    result.startStateId = -1;
  }

  return result;
}
} // namespace lexer
} // namespace metalyzer
