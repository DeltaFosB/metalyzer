#include <metalyzer/lexer/Compressor.hpp>
#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/Minimizer.hpp>
#include <metalyzer/lexer/NFA.hpp>
#include <metalyzer/lexer/PowerSetConstructor.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>
#include <metalyzer/lexer/TransTable.hpp>

#include <iomanip> // Needed for std::setw to align the table columns
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

void printTransTable(const metalyzer::TransTable &tt) {
  std::cout << "\n=== Compressed Transition Table ===\n";
  std::cout << "Start State Index: " << tt.startStateId << "\n";
  std::cout << "Rows (States): " << tt.table.size() << "\n";
  std::cout << "Cols (Chars):  " << (tt.table.empty() ? 0 : tt.table[0].size())
            << "\n";
  std::cout << "-----------------------------------\n";

  // 1. Scan to find used columns
  std::vector<int> usedChars;
  if (!tt.table.empty()) {
    for (int c = 0; c < tt.table[0].size(); ++c) {
      bool used = false;
      for (const auto &row : tt.table) {
        if (row[c] != -1) {
          used = true;
          break;
        }
      }
      if (used)
        usedChars.push_back(c);
    }
  }

  // 2. Print Header
  std::cout << "State | Acc? |";
  for (int c : usedChars) {
    std::cout << " '" << (char)c << "' |";
  }
  std::cout << "\n";
  std::cout << "------|------|";
  for (size_t i = 0; i < usedChars.size(); ++i)
    std::cout << "----|";
  std::cout << "\n";

  // 3. Print Rows
  for (size_t i = 0; i < tt.table.size(); ++i) {
    std::cout << std::setw(5) << i << " | "
              << (tt.isAccepting[i] ? "Yes " : "No  ") << " |";

    for (int c : usedChars) {
      int target = tt.table[i][c];
      if (target == -1) {
        std::cout << "  - |";
      } else {
        std::cout << std::setw(3) << target << " |";
      }
    }
    std::cout << "\n";
  }
  std::cout << "-----------------------------------\n";
}

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

  metalyzer::DFAContext min_ctx;
  metalyzer::Minimizer minimizer;
  metalyzer::DFA mdfa = minimizer.minimize(dfa, min_ctx);

  std::cout << "\n=== Minimized DFA ===\n";
  if (mdfa.start != nullptr) {
    printDFAInfo(pattern, mdfa);
  }

  metalyzer::Compressor compressor;
  metalyzer::TransTable cdfa = compressor.compress(mdfa);

  printTransTable(cdfa);

  return 0;
}
