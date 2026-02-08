#include <metalyzer/generator/Generator.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/TransTable.hpp>

#include <iomanip>
#include <iostream>
#include <vector>

// Helper to print the compressed table
void printTransTable(const metalyzer::lexer::TransTable &tt) {
  std::cout << "\n=== Compressed Transition Table ===\n";
  std::cout << "Start State Index: " << tt.startStateId << "\n";
  std::cout << "Rows (States): " << tt.table.size() << "\n";
  std::cout << "Cols (Chars):  " << (tt.table.empty() ? 0 : tt.table[0].size())
            << "\n";
  std::cout << "-----------------------------------\n";

  // 1. Scan to find used columns (for cleaner output)
  std::vector<int> usedChars;
  if (!tt.table.empty()) {
    for (size_t c = 0; c < tt.table[0].size(); ++c) {
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
  std::cout << " State | Rule |";
  for (int c : usedChars) {
    if (c >= 32 && c <= 126)
      std::cout << " '" << (char)c << "' |";
    else
      std::cout << " x" << std::hex << c << std::dec << " |";
  }
  std::cout << "\n";

  std::cout << "-------|------|";
  for (size_t i = 0; i < usedChars.size(); ++i)
    std::cout << "-----|";
  std::cout << "\n";

  // 3. Print Rows
  for (size_t i = 0; i < tt.table.size(); ++i) {
    std::cout << std::setw(6) << i << " | ";

    int ruleId = tt.acceptRuleIds[i];
    if (ruleId != -1) {
      std::cout << std::setw(4) << ruleId << " |";
    } else {
      std::cout << "   - |";
    }

    // Transitions
    for (int c : usedChars) {
      int target = tt.table[i][c];
      if (target == -1) {
        std::cout << "  -  |";
      } else {
        std::cout << std::setw(4) << target << " |";
      }
    }
    std::cout << "\n";
  }
  std::cout << "-----------------------------------\n";
}

int main() {
  std::cout << "=== Metalyzer Multi-Rule Test ===\n";

  // 1. Initialize Builder
  metalyzer::lexer::LexerBuilder builder;

  // 2. Add Rules
  std::cout << "Adding rules...\n";
  std::cout << "Rule 1: 'if'\n";
  builder.addRule("if", 1);

  std::cout << "Rule 2: 'while'\n";
  builder.addRule("while", 2);

  std::cout << "Rule 3: Integers [0-9]+\n";
  builder.addRule("[0-9]+", 3);

  std::cout << "Rule 4: Identifiers [a-z]+\n";
  builder.addRule("[a-z]+", 4);

  // 3. Build (Compile NFA -> DFA -> Minimize -> Compress)
  std::cout << "Building Lexer...\n";
  metalyzer::lexer::TransTable table = builder.build();

  // 4. Inspect Table
  printTransTable(table);

  // 5. Generate C++ Code
  std::cout << "\n=== Generating C++ Lexer Code ===\n";

  metalyzer::generator::Generator::Config config;
  config.outputDir = "generated_lexer";
  config.className = "MyLexer";
  config.namespaceName = "user_code";

  metalyzer::generator::Generator gen(config);
  gen.generate(table);

  std::cout << "Done! Driver code and lexer generated in '" << config.outputDir
            << "/'\n";

  return 0;
}
