#include <metalyzer/lexer/ThompsonConstructor.hpp>
#include <metalyzer/lexer/NFA.hpp>
/*
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/TransTable.hpp>
*/

#include <iostream>

void printNFAInfo(const std::string& pattern, const metalyzer::NFA& nfa) {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Regex Pattern: " << pattern << std::endl;
    std::cout << "Start State ID: " << nfa.start->id << std::endl;
    std::cout << "End State ID:   " << nfa.end->id << std::endl;
    std::cout << "Total States:   " << nfa.allStates.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;
}

using namespace std;

int main(){

  metalyzer::NFAContext context;
  metalyzer::ThompsonConstructor compiler(context);

  std::string pattern = "(a|b)*c";
  metalyzer::NFA resultNFA = compiler.build(pattern);

  if (resultNFA.start != nullptr) {
    printNFAInfo(pattern, resultNFA);
  } else {
    std::cerr << "Failed to build NFA." << std::endl;
  }

  /*
  metalyzer::PowerSetConstructor psconst;
  metalyzer::Minimizer minimizer;
  metalyzer::Compressor compressor;

  metalyzer::DFA dfa = psconst.convert(nfa);
  metalyzer::DFA mdfa = minimizer.minimize(dfa);
  metalyzer::TransTable cdfa = compressor.compress(dfa);
  */

  return 0;
}
