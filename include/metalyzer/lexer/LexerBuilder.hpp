#pragma once

#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/NFA.hpp>
#include <metalyzer/lexer/TransTable.hpp>
#include <string>
#include <vector>

namespace metalyzer {
namespace lexer {

class LexerBuilder {
public:
  void addRule(const std::string &regex, int ruleId);
  TransTable build(const std::string &spec_name = "<spec_name>");

private:
  struct Rule {
    std::string regex;
    int id;
  };

  std::vector<Rule> rules;

  NFAContext nfaCtx;
  DFAContext dfaCtx;
  DFAContext minDfaCtx;
};

} // namespace lexer
} // namespace metalyzer
