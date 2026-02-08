#include <algorithm>
#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <unordered_map>

namespace metalyzer {
namespace lexer {

DFA Minimizer::minimize(const DFA &dfa, DFAContext &ctx) {
  std::set<char> alphabet = getAlphabet(dfa);
  auto partitions = initPartitions(dfa);

  while (true) {
    auto nextPartitions = refinePartitions(partitions, alphabet);

    if (nextPartitions.size() == partitions.size()) {
      break;
    }
    partitions = nextPartitions;
  }

  return makeDFA(dfa, partitions, ctx);
}

std::set<char> Minimizer::getAlphabet(const DFA &dfa) {
  std::set<char> alphabet;
  for (DFAState *state : dfa.allStates) {
    for (auto const &[symbol, target] : state->transitions) {
      if (symbol != '\0') {
        alphabet.insert(symbol);
      }
    }
  }
  return alphabet;
}

std::vector<std::vector<DFAState *>> Minimizer::initPartitions(const DFA &dfa) {
  std::map<int, std::vector<DFAState *>> ruleGroups;

  for (DFAState *state : dfa.allStates) {
    ruleGroups[state->acceptRuleId].push_back(state);
  }

  std::vector<std::vector<DFAState *>> partitions;
  for (auto const &[id, group] : ruleGroups) {
    partitions.push_back(group);
  }

  return partitions;
}

std::vector<std::vector<DFAState *>> Minimizer::refinePartitions(
    const std::vector<std::vector<DFAState *>> &partitions,
    const std::set<char> &alphabet) {

  std::unordered_map<int, int> stateToGroup;
  for (size_t i = 0; i < partitions.size(); ++i) {
    for (DFAState *s : partitions[i]) {
      stateToGroup[s->id] = static_cast<int>(i);
    }
  }

  std::vector<std::vector<DFAState *>> nextPartitions;

  for (const auto &partitionSet : partitions) {
    if (partitionSet.size() <= 1) {
      nextPartitions.push_back(partitionSet);
      continue;
    }

    std::vector<std::vector<DFAState *>> splitSets;
    if (splitSet(partitionSet, alphabet, stateToGroup, splitSets)) {
      nextPartitions.insert(nextPartitions.end(), splitSets.begin(),
                            splitSets.end());
    } else {
      nextPartitions.push_back(partitionSet);
    }
  }
  return nextPartitions;
}
bool Minimizer::splitSet(
    const std::vector<DFAState *> &currentGroup, const std::set<char> &alphabet,
    const std::unordered_map<int, int> &stateToGroup,
    std::vector<std::vector<DFAState *>> &outputSubGroups) {
  for (char c : alphabet) {
    std::map<int, std::vector<DFAState *>> buckets;
    for (DFAState *s : currentGroup) {
      int targetGroup = -1;
      auto transIt = s->transitions.find(c);

      if (transIt != s->transitions.end()) {
        DFAState *targetState = transIt->second;

        auto groupIt = stateToGroup.find(targetState->id);
        if (groupIt != stateToGroup.end()) {
          targetGroup = groupIt->second;
        }
      }
      buckets[targetGroup].push_back(s);
    }

    if (buckets.size() > 1) {
      for (auto const &[id, subgroup] : buckets) {
        outputSubGroups.push_back(subgroup);
      }
      return true;
    }
  }
  return false;
}

DFA Minimizer::makeDFA(const DFA &originalDFA,
                       const std::vector<std::vector<DFAState *>> &partitions,
                       DFAContext &ctx) {
  DFA dfa;
  std::unordered_map<int, DFAState *> oldToNew;
  std::vector<DFAState *> newStates;

  for (const auto &partition : partitions) {
    DFAState *newState = ctx.createState();

    DFAState *representative = partition[0];
    newState->acceptRuleId = representative->acceptRuleId;

    for (DFAState *oldState : partition) {
      oldToNew[oldState->id] = newState;

      if (oldState == originalDFA.start) {
        dfa.start = newState;
      }
    }
    newStates.push_back(newState);
  }

  for (size_t i = 0; i < partitions.size(); ++i) {
    DFAState *newState = newStates[i];
    DFAState *representative = partitions[i][0];

    for (auto const &[symbol, oldTarget] : representative->transitions) {
      newState->transitions[symbol] = oldToNew[oldTarget->id];
    }
  }

  dfa.allStates = std::move(newStates);

  return dfa;
}

} // namespace lexer
} // namespace metalyzer
