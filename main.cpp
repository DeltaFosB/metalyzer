#include <metalyzer/frontend/LexerSpec.hpp>
#include <metalyzer/frontend/SpecParser.hpp>
#include <metalyzer/generator/Generator.hpp>
#include <metalyzer/lexer/LexerBuilder.hpp>
#include <metalyzer/lexer/TransTable.hpp>

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

int main() {
  std::cout << "=== Metalyzer Parser & Builder Test ===\n";

  std::string mockFileContent = R"(
%{
#include <iostream>
#include <string>
#include <sstream>

// Simulating a real token system for a micro-language
enum class TokenType {
    TOKEN_EOF = -1,
    TOKEN_ERROR = -2,
    TOK_INT = 1,
    TOK_FLOAT = 2,
    TOK_IF = 3,
    TOK_ELSE = 4,
    TOK_WHILE = 5,
    TOK_ID = 6,
    TOK_ASSIGN = 7,
    TOK_EQUAL = 8
};
%}

%%

[0-9]+          { return (int)TokenType::TOK_INT; }
[0-9]+\.[0-9]+  { return (int)TokenType::TOK_FLOAT; }
if              { return (int)TokenType::TOK_IF; }
else            { return (int)TokenType::TOK_ELSE; }
while           { return (int)TokenType::TOK_WHILE; }
==              { return (int)TokenType::TOK_EQUAL; }
=               { return (int)TokenType::TOK_ASSIGN; }
[a-z]+          { return (int)TokenType::TOK_ID; }

%%

// --- EMBEDDED TEST DRIVER SUITE ---

std::string get_token_name(int token_id) {
    switch (static_cast<TokenType>(token_id)) {
        case TokenType::TOKEN_EOF:   return "EOF";
        case TokenType::TOKEN_ERROR: return "ERROR";
        case TokenType::TOK_INT:     return "TOK_INT";
        case TokenType::TOK_FLOAT:   return "TOK_FLOAT";
        case TokenType::TOK_IF:      return "TOK_IF";
        case TokenType::TOK_ELSE:    return "TOK_ELSE";
        case TokenType::TOK_WHILE:   return "TOK_WHILE";
        case TokenType::TOK_ID:      return "TOK_ID";
        case TokenType::TOK_ASSIGN:  return "TOK_ASSIGN";
        case TokenType::TOK_EQUAL:   return "TOK_EQUAL";
        default:                     return "UNKNOWN";
    }
}

int main() {
    // Explicitly pull your custom generated namespace configuration into scope
    using namespace user_code;

    std::cout << "========================================" << std::endl;
    std::cout << "   RUNNING GENERATED RUNTIME TESTING    " << std::endl;
    std::cout << "========================================" << std::endl;

    std::string testInput = "if x == 42.12 else while x = 7";
    std::stringstream testStream(testInput);

    std::cout << "Input Stream: \"" << testInput << "\"\n\n";

    // Now correctly resolves to generated::Lexer based on your default Config properties
    MyLexer lexer(testStream);

    std::string lexeme;
    int token;

    while (lexer.hasMore()) {
        token = lexer.nextToken(lexeme);
        
        std::cout << "Matched -> [" << get_token_name(token) 
                  << "] with value: \"" << lexeme << "\"" << std::endl;

        if (token == (int)TokenType::TOKEN_ERROR) {
            std::cerr << "Stopping scan due to lexical error!" << std::endl;
            return 1;
        }
        if (token == (int)TokenType::TOKEN_EOF) {
            break;
        }
    }

    std::cout << "\nScanning complete! Pipeline verified successfully." << std::endl;
    return 0;
}
)";

  std::cout << "Input File Content:\n" << mockFileContent << "\n\n";

  std::cout << ">>> Parsing .mz content...\n";
  metalyzer::frontend::SpecParser parser;
  metalyzer::frontend::LexerSpec spec;

  try {
    spec = parser.parse(mockFileContent);
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
