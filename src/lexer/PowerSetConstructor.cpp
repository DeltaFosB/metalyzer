#include <map>
#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/NFA.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <queue>
#include <stack>

namespace metalyzer {

std::set<NFAState *>
PowerSetConstructor::computeEpsilonClosure(NFAState *state) {
  if (closureCache.count(state->id)) {
    return closureCache[state->id];
  }

  std::set<NFAState *> closure;
  std::stack<NFAState *> st;

  st.push(state);
  closure.insert(state);

  while (!st.empty()) {
    NFAState *curr = st.top();
    st.pop();

    auto it = curr->transitions.find('\0');
    if (it != curr->transitions.end()) {
      for (NFAState *next : it->second) {
        if (closure.find(next) == closure.end()) {
          closure.insert(next);
          st.push(next);
        }
      }
    }
  }

  return closureCache[state->id] = closure;
}

std::set<NFAState *>
PowerSetConstructor::computeEpsilonClosure(const std::set<NFAState *> &states) {
  std::set<NFAState *> totalClosure;
  for (NFAState *s : states) {
    std::set<NFAState *> singleClosure = computeEpsilonClosure(s);
    totalClosure.insert(singleClosure.begin(), singleClosure.end());
  }
  return totalClosure;
}

std::set<char> PowerSetConstructor::getAlphabet(const NFA &nfa) {
  std::set<char> alphabet;
  for (NFAState *state : nfa.allStates) {
    for (auto const &[symbol, targets] : state->transitions) {
      if (symbol != '\0') {
        alphabet.insert(symbol);
      }
    }
  }
  return alphabet;
}

DFA PowerSetConstructor::convert(const NFA &nfa) {

  DFA dfa;
  std::map<std::set<NFAState *>, DFAState *> seenSets;
  std::queue<std::set<NFAState *>> worklist;

  std::set<NFAState *> startSet = computeEpsilonClosure(nfa.start);
  DFAState *d0 = ctx.createState();

  seenSets[startSet] = d0;
  worklist.push(startSet);
  dfa.start = d0;
  for (NFAState *nfaS : startSet) {
    if (nfaS->isAccepting) {
      d0->isAccepting = true;
      break;
    }
  }
  dfa.allStates.push_back(d0);

  std::set<char> alphabet = getAlphabet(nfa);

  while (!worklist.empty()) {
    std::set<NFAState *> T = worklist.front();
    worklist.pop();
    DFAState *currentDFAState = seenSets[T];

    for (char c : alphabet) {
      std::set<NFAState *> transSet;
      for (auto st : T) {
        auto it = st->transitions.find(c);
        if (it != st->transitions.end()) {
          const std::vector<NFAState *> &targets = it->second;
          transSet.insert(targets.begin(), targets.end());
        }
      }
      if (transSet.empty())
        continue;
      std::set<NFAState *> U = computeEpsilonClosure(transSet);

      if (seenSets.find(U) == seenSets.end()) {
        DFAState *newState = ctx.createState();

        for (NFAState *nfaS : U) {
          if (nfaS->isAccepting) {
            newState->isAccepting = true;
            break;
          }
        }

        seenSets[U] = newState;
        worklist.push(U);
        dfa.allStates.push_back(newState);
      }

      currentDFAState->transitions[c] = seenSets[U];
    }
  }

  return dfa;
}

} // namespace metalyzer
