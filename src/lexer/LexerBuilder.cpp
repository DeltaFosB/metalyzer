#include <iostream>
#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/Profiler.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>

namespace metalyzer {
namespace lexer {

void LexerBuilder::addRule(const std::string &regex, int ruleId) {
  rules.push_back({regex, ruleId});
}

TransTable LexerBuilder::build() {
  if (rules.empty())
    return TransTable{};

  // Instantiate our telemetry carrier at the entry boundary
  ProfilingMetrics metrics;

  // 1. Create the "Super Start State"
  NFAState *superStart = nfaCtx.createState();

  // Container for all NFA states
  std::vector<NFAState *> allNFAStates;
  allNFAStates.push_back(superStart);

  // 2. Build and Merge all Rules
  ThompsonConstructor thompson(nfaCtx);

  for (const auto &rule : rules) {
    // Build isolated NFA for this rule
    NFA ruleNFA = thompson.build(rule.regex, rule.id);

    if (ruleNFA.start) {
      // Connect Super Start -> Rule Start (Epsilon Transition)
      // '\0' represents Epsilon in our system
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

  // Boundary Extraction Guardrail 1: Capture true Thompson states including
  // epsilons
  metrics.nfa_states = static_cast<uint32_t>(combinedNFA.allStates.size());

  // 4. Convert to DFA
  PowerSetConstructor powerSet(dfaCtx);
  DFA dfa = powerSet.convert(combinedNFA);

  // Boundary Extraction Guardrail 2: Capture unminimized powerset expansion
  // count
  metrics.dfa_initial_states = static_cast<uint32_t>(dfa.allStates.size());

  // 5. Minimize DFA
  Minimizer minimizer;
  DFA minDFA = minimizer.minimize(dfa, minDfaCtx);

  // Boundary Extraction Guardrail 3: Capture finalized optimized nodes count
  metrics.dfa_minimized_states = static_cast<uint32_t>(minDFA.allStates.size());

  // 6. Compress to Table
  Compressor compressor;
  TransTable compressedTable = compressor.compress(minDFA);

  // Boundary Extraction Guardrail 4: Fetch actual matrix dimensional
  // allocations cleanly
  if (!compressedTable.table.empty()) {
    metrics.alphabet_columns =
        static_cast<uint32_t>(compressedTable.table[0].size());
  } else {
    metrics.alphabet_columns = 0;
  }
  metrics.type_size_bytes = static_cast<uint32_t>(sizeof(int));

  // 7. Emit Content-Rich Custom Profiling Block to standard out
#ifdef ENABLE_PROFILING
  // You can pass the spec file path or variable identifier string here
  printProfilingReport("Generated Specification Target", metrics);
#endif

  return compressedTable;
}

} // namespace lexer
} // namespace metalyzer
