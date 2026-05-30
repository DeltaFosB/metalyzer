#pragma once

#include <filesystem>
#include <metalyzer/frontend/LexerSpec.hpp>
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

    // Low-overhead toggle to instrument generated AOT runtime components
    bool enableRuntimeObservability = false;
  };

  explicit Generator(const Config &config);

  void generate(const metalyzer::lexer::TransTable &table,
                const metalyzer::frontend::LexerSpec &spec);

private:
  Config config;

  std::string serializeTable(const metalyzer::lexer::TransTable &table) const;

  std::string
  serializeAcceptance(const metalyzer::lexer::TransTable &table) const;

  void writeToFile(const std::string &filename,
                   const std::string &content) const;
};

} // namespace generator
} // namespace metalyzer
