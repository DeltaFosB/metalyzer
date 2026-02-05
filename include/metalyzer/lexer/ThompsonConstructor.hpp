#pragma once

#include <metalyzer/lexer/NFA.hpp>

namespace metalyzer {
class ThompsonConstructor {
public:
  explicit ThompsonConstructor(NFAContext &context) : ctx(context) {};
  NFA build(const std::string &regexPattern);

private:
  NFAContext &ctx;
  NFA createBasic(char symbol);
  NFA createUnion(NFA left, NFA right);
  NFA createConcat(NFA left, NFA right);
  NFA createStar(NFA base);

  NFA createPlus(NFA base);
  NFA createOptional(NFA base);

  std::string preprocess(const std::string &regex);
  std::string shunt_yard(const std::string &regex);

  int getPrecedence(char op);
};
} // namespace metalyzer
