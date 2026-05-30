#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/Profiler.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>

#include <iostream>

namespace metalyzer {
namespace lexer {

void LexerBuilder::addRule(const std::string &regex, int ruleId) {
  rules.push_back({regex, ruleId});
}

TransTable LexerBuilder::build(const std::string &spec_name) {
  if (rules.empty())
    return TransTable{};

  ProfilingMetrics metrics;

  // 1. Create the "Super Start State"
  NFAState *superStart = nfaCtx.createState();

  // Container for all NFA states
  std::vector<NFAState *> allNFAStates;
  allNFAStates.push_back(superStart);

  // 2. Build and Merge all Rules
  ThompsonConstructor thompson(nfaCtx);

  for (const auto &rule : rules) {
    NFA ruleNFA = thompson.build(rule.regex, rule.id);

    if (ruleNFA.start) {
      // Connect Super Start -> Rule Start (Epsilon Transition)
      superStart->transitions['\0'].push_back(ruleNFA.start);

      // Collect all states
      allNFAStates.insert(allNFAStates.end(), ruleNFA.allStates.begin(),
                          ruleNFA.allStates.end());
    }
  }

  // 3. Construct the Combined NFA Wrapper
  NFA combinedNFA;
  combinedNFA.start = superStart;
  combinedNFA.allStates = allNFAStates;

  metrics.nfa_states = static_cast<uint32_t>(combinedNFA.allStates.size());

  // 4. Convert to DFA via powerset expansion
  PowerSetConstructor powerSet(dfaCtx);
  DFA dfa = powerSet.convert(combinedNFA);

  metrics.dfa_initial_states = static_cast<uint32_t>(dfa.allStates.size());

  // 5. Minimize DFA using Hopcroft's partition refinement loops
  Minimizer minimizer;
  DFA minDFA = minimizer.minimize(dfa, minDfaCtx);

  metrics.dfa_minimized_states = static_cast<uint32_t>(minDFA.allStates.size());

  // 6. Compress to Flat 2D Transition Matrix Table
  Compressor compressor;
  TransTable compressedTable = compressor.compress(minDFA);

  if (!compressedTable.table.empty()) {
    metrics.alphabet_columns =
        static_cast<uint32_t>(compressedTable.table[0].size());
  } else {
    metrics.alphabet_columns = 0;
  }

  metrics.type_size_bytes = static_cast<uint32_t>(sizeof(int));

  // 7. Process analytics and serialize profile data
  printProfilingReport(spec_name, metrics, "generated_lexer");

  return compressedTable;
}

} // namespace lexer
} // namespace metalyzer
