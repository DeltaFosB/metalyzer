#include <iostream>
#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>

namespace metalyzer {
namespace lexer {

void LexerBuilder::addRule(const std::string &regex, int ruleId) {
  rules.push_back({regex, ruleId});
}

TransTable LexerBuilder::build() {
  if (rules.empty())
    return TransTable{};

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

  // 4. Convert to DFA
  PowerSetConstructor powerSet(dfaCtx);
  DFA dfa = powerSet.convert(combinedNFA);

  // 5. Minimize DFA
  Minimizer minimizer;
  DFA minDFA = minimizer.minimize(dfa, minDfaCtx);

  // 6. Compress to Table
  Compressor compressor;
  return compressor.compress(minDFA);
}

} // namespace lexer
} // namespace metalyzer
