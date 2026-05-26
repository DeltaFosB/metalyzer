#include <metalyzer/frontend/LexerSpec.hpp>
#include <metalyzer/frontend/SpecParser.hpp>
#include <metalyzer/generator/Generator.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/TransTable.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void printTransTable(const metalyzer::lexer::TransTable &tt) {
  std::cout << "\n=== Compressed Transition Table ===\n";
  std::cout << "Start State Index: " << tt.startStateId << "\n";
  std::cout << "Rows (States): " << tt.table.size() << "\n";
  std::cout << "Cols (Chars):  " << (tt.table.empty() ? 0 : tt.table[0].size())
            << "\n";
  std::cout << "-----------------------------------\n";

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

  std::cout << " State | Rule |";
  for (int c : usedChars) {
    if (c >= 32 && c <= 126)
      std::cout << " '" << (char)c << "' |";
    else
      std::cout << " x" << std::hex << c << std::dec << " |";
  }
  std::cout << "\n-------|------|";
  for (size_t i = 0; i < usedChars.size(); ++i)
    std::cout << "-----|";
  std::cout << "\n";

  for (size_t i = 0; i < tt.table.size(); ++i) {
    std::cout << std::setw(6) << i << " | ";
    int ruleId = tt.acceptRuleIds[i];
    if (ruleId != -1)
      std::cout << std::setw(4) << ruleId << " |";
    else
      std::cout << "   - |";

    for (int c : usedChars) {
      int target = tt.table[i][c];
      if (target == -1)
        std::cout << "  -  |";
      else
        std::cout << std::setw(4) << target << " |";
    }
    std::cout << "\n";
  }
  std::cout << "-----------------------------------\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: ./metalyzer_app <path_to_spec.mz>\n";
    exit(1);
  }

  std::ifstream mzfilestream(argv[1]);

  if (!mzfilestream.is_open()) {
    std::cerr << "Filestream not open\n";
    exit(1);
  }

  std::stringstream ss;
  ss << mzfilestream.rdbuf();
  std::string fileContent = ss.str();

  std::cout << "=== Metalyzer Parser & Builder Test ===\n";

  std::cout << "Input File Content:\n" << fileContent << "\n\n";

  std::cout << ">>> Parsing .mz content...\n";
  metalyzer::frontend::SpecParser parser;
  metalyzer::frontend::LexerSpec spec;

  try {
    spec = parser.parse(fileContent);
  } catch (const std::exception &e) {
    std::cerr << "Parser Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << ">>> Parse Success!\n";
  std::cout << "Header Code Size: " << spec.headerCode.size() << " chars\n";
  std::cout << "User Code Size:   " << spec.userCode.size() << " chars\n";
  std::cout << "Found " << spec.rules.size() << " rules:\n";

  for (const auto &rule : spec.rules) {
    std::cout << "  [" << rule.priority << "] Regex: '" << rule.regex
              << "' -> Action: '" << rule.actionCode << "'\n";
  }

  std::cout << "\n>>> Building Lexer Engine...\n";
  metalyzer::lexer::LexerBuilder builder;

  for (const auto &rule : spec.rules) {
    builder.addRule(rule.regex, rule.priority);
  }

  metalyzer::lexer::TransTable table = builder.build();
  printTransTable(table);

  std::cout << "\n>>> Generating C++ Lexer Code...\n";
  metalyzer::generator::Generator::Config config;
  config.outputDir = "generated_lexer";
  config.className = "MyLexer";
  config.namespaceName = "user_code";

  metalyzer::generator::Generator gen(config);

  gen.generate(table, spec);

  std::cout << "Done! Output in 'generated_lexer/'\n";

  return 0;
}
