#pragma once
#include <metalyzer/frontend/LexerSpec.hpp>
#include <string>

namespace metalyzer {
namespace frontend {

class SpecParser {
public:
  LexerSpec parse(const std::string &content);

private:
  void parseDefinitions(const std::string &section, LexerSpec &spec);

  void parseRules(const std::string &section, LexerSpec &spec);

  void parseUserCode(const std::string &section, LexerSpec &spec);

  std::string trim(const std::string &str);
};

} // namespace frontend
} // namespace metalyzer
