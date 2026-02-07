#include <metalyzer/generator/Generator.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace metalyzer {
namespace generator {

static void replaceAll(std::string &str, const std::string &from,
                       const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

Generator::Generator(const Config &config) : config(config) {}

std::string Generator::serializeTable(const TransTable &table) const {
  std::stringstream ss;
  ss << "{\n";
  for (size_t i = 0; i < table.table.size(); ++i) {
    ss << "    {";
    for (int ch = 0; ch < 128; ++ch) {
      ss << table.table[i][ch];
      if (ch < 127)
        ss << ", ";
    }
    ss << "}";
    if (i < table.table.size() - 1)
      ss << ",";
    ss << "\n";
  }
  ss << "}";
  return ss.str();
}

std::string Generator::serializeAcceptance(const TransTable &table) const {
  std::stringstream ss;
  ss << "{";
  for (size_t i = 0; i < table.isAccepting.size(); ++i) {
    ss << (table.isAccepting[i] ? "true" : "false");
    if (i < table.isAccepting.size() - 1) {
      ss << ", ";
    }
  }
  ss << "}";
  return ss.str();
}

void Generator::writeToFile(const std::string &filename,
                            const std::string &content) const {
  std::filesystem::path fullPath = config.outputDir / filename;

  if (!std::filesystem::exists(config.outputDir)) {
    std::filesystem::create_directories(config.outputDir);
  }

  std::ofstream outFile(fullPath);
  if (outFile.is_open()) {
    outFile << content;
    outFile.close();
    std::cout << "Generated: " << fullPath << std::endl;
  } else {
    std::cerr << "Error: Could not write to " << fullPath << std::endl;
  }
}

void Generator::generate(const TransTable &table) {
  std::string rowsStr = std::to_string(table.table.size());
  std::string tableData = serializeTable(table);
  std::string acceptData = serializeAcceptance(table);
  std::string startState = std::to_string(table.startStateId);

  std::string header = R"(
#pragma once
#include <string>
#include <iostream>

namespace @NAMESPACE@ {

class @CLASS_NAME@ {
public:
    explicit @CLASS_NAME@(std::istream& input);
    
    std::string nextToken();
    bool hasMore() const;

private:
    std::istream& input;
    
    // Hardcoded Transition Table
    static const int TABLE_ROWS = @TABLE_ROWS@;
    static const int TRANS_TABLE[TABLE_ROWS][128];
    static const bool IS_ACCEPTING[TABLE_ROWS];
    static const int START_STATE = @START_STATE@;
};

} // namespace @NAMESPACE@
)";

  // --- SOURCE TEMPLATE ---
  std::string source = R"(
#include "@CLASS_NAME@.hpp"
#include <cctype>

namespace @NAMESPACE@ {

// --- STATIC DATA ---
const int @CLASS_NAME@::TRANS_TABLE[@TABLE_ROWS@][128] = @TABLE_DATA@;

const bool @CLASS_NAME@::IS_ACCEPTING[@TABLE_ROWS@] = @ACCEPT_DATA@;

// --- IMPLEMENTATION ---
@CLASS_NAME@::@CLASS_NAME@(std::istream& in) : input(in) {}

bool @CLASS_NAME@::hasMore() const {
    return input.good() && input.peek() != EOF;
}

std::string @CLASS_NAME@::nextToken() {
    std::string lexeme;
    int currentState = START_STATE;
    
    // 1. Initial Check
    if (!hasMore()) return "";

    // 2. Skip Whitespace
    while (hasMore() && std::isspace(input.peek())) {
        input.get(); 
    }

    // 3. Post-Whitespace Check (Prevent returning empty if file ends in space)
    if (!hasMore()) return "";

    // 4. Driver Loop
    while (hasMore()) {
        char c = input.peek();
        
        // Ensure char is within 0-127
        if (c < 0 || c > 127) break; 

        int nextState = TRANS_TABLE[currentState][(int)c];

        if (nextState == -1) {
            // No transition possible. Stop here.
            break;
        }

        // Consume and Advance
        lexeme += c;
        input.get();
        currentState = nextState;
    }

    // 5. Final Decision
    if (IS_ACCEPTING[currentState]) {
        return lexeme;
    } else {
        // Error or partial match failure
        if (!lexeme.empty()) {
            std::cerr << "Lexical Error: Unexpected input '" << lexeme << "'" << std::endl;
        }
        return ""; 
    }
}

} // namespace @NAMESPACE@
)";

  auto processTemplate = [&](std::string text) -> std::string {
    replaceAll(text, "@NAMESPACE@", config.namespaceName);
    replaceAll(text, "@CLASS_NAME@", config.className);
    replaceAll(text, "@TABLE_ROWS@", rowsStr);
    replaceAll(text, "@TABLE_DATA@", tableData);
    replaceAll(text, "@ACCEPT_DATA@", acceptData);
    replaceAll(text, "@START_STATE@", startState);
    return text;
  };

  std::string headerContent = processTemplate(header);
  std::string sourceContent = processTemplate(source);

  writeToFile(config.className + ".hpp", headerContent);
  writeToFile(config.className + ".cpp", sourceContent);
}

} // namespace generator
} // namespace metalyzer
