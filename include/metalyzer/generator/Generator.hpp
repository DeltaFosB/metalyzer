#pragma once

#include <filesystem>
#include <metalyzer/lexer/TransTable.hpp>
#include <string>

namespace metalyzer {
namespace generator {

class Generator {
public:
  struct Config {
    std::string className = "Lexer";
    std::string namespaceName = "generated";
    std::filesystem::path outputDir = "generated";
  };

  explicit Generator(const Config &config);

  void generate(const TransTable &table);

private:
  Config config;

  std::string serializeTable(const TransTable &table) const;

  std::string serializeAcceptance(const TransTable &table) const;

  void writeToFile(const std::string &filename,
                   const std::string &content) const;
};

} // namespace generator
} // namespace metalyzer
