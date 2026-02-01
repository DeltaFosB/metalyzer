#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/NFA.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>
/*
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/TransTable.hpp>
*/

#include <iostream>

void printNFAInfo(const std::string &pattern, const metalyzer::NFA &nfa) {
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "Regex Pattern: " << pattern << std::endl;
  std::cout << "Start State ID: " << nfa.start->id << std::endl;
  std::cout << "End State ID:   " << nfa.end->id << std::endl;
  std::cout << "Total States:   " << nfa.allStates.size() << std::endl;
  std::cout << "------------------------------------------" << std::endl;
}

void printDFAInfo(const std::string &pattern, const metalyzer::DFA &dfa) {
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "DFA Regex Pattern: " << pattern << std::endl;
  std::cout << "Start State ID:    " << dfa.start->id << std::endl;
  std::cout << "Total States:      " << dfa.allStates.size() << std::endl;
  std::cout << "------------------------------------------" << std::endl;

  for (auto *state : dfa.allStates) {
    std::cout << "State " << state->id
              << (state->isAccepting ? " (Accepting)" : "") << ":" << std::endl;
    for (const auto &[symbol, target] : state->transitions) {
      std::cout << "  '" << symbol << "' -> State " << target->id << std::endl;
    }
  }
  std::cout << "------------------------------------------" << std::endl;
}

using namespace std;

int main() {

  metalyzer::NFAContext nfa_ctx;
  metalyzer::ThompsonConstructor compiler(nfa_ctx);

  std::string pattern = "(a|b)*c";
  metalyzer::NFA resultNFA = compiler.build(pattern);

  if (resultNFA.start != nullptr) {
    printNFAInfo(pattern, resultNFA);
  } else {
    std::cerr << "Failed to build NFA." << std::endl;
    return 1;
  }

  metalyzer::DFAContext dfa_ctx;
  metalyzer::PowerSetConstructor psconst(dfa_ctx);

  metalyzer::DFA dfa = psconst.convert(resultNFA);

  if (dfa.start != nullptr) {
    printDFAInfo(pattern, dfa);
  } else {
    std::cerr << "Failed to convert NFA to DFA." << std::endl;
    return 1;
  }
  /*

  metalyzer::Minimizer minimizer;
  metalyzer::Compressor compressor;

  metalyzer::DFA mdfa = minimizer.minimize(dfa);
  metalyzer::TransTable cdfa = compressor.compress(dfa);
  */

  return 0;
}
