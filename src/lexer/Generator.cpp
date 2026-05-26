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

std::string
Generator::serializeTable(const metalyzer::lexer::TransTable &table) const {
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

std::string Generator::serializeAcceptance(
    const metalyzer::lexer::TransTable &table) const {
  std::stringstream ss;
  ss << "{";
  for (size_t i = 0; i < table.acceptRuleIds.size(); ++i) {
    ss << table.acceptRuleIds[i];
    if (i < table.acceptRuleIds.size() - 1) {
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

void Generator::generate(const metalyzer::lexer::TransTable &table,
                         const metalyzer::frontend::LexerSpec &spec) {
  std::string rowsStr = std::to_string(table.table.size());
  std::string tableData = serializeTable(table);
  std::string acceptData = serializeAcceptance(table);
  std::string startState = std::to_string(table.startStateId);

  std::stringstream ss;

  ss << "switch(lastGoodRule) {\n";

  for (const auto &rule : spec.rules) {
    int case_idx = rule.priority;
    std::string case_code = rule.actionCode;

    ss << "           case " << case_idx << ": " << case_code << " break;\n";
  }
  ss << "         }";

  // --- HEADER TEMPLATE ---
  std::string header = R"(
#pragma once
#include <string>
#include <iostream>
@HEADER_CODE@

namespace @NAMESPACE@ {

class @CLASS_NAME@ {
public:
    explicit @CLASS_NAME@(std::istream& input);
    
    // Returns -1 on EOF, -2 on Error.
    int nextToken(std::string& outLexeme);
    
    bool hasMore() const;

    const int getLine() { return currentLine; }
    const int getCol() { return tokenStartCol; }

private:
    std::istream& input;
    
    int currentLine = 1, currentCol = 1;
    int tokenStartCol = 1;

    static const int TABLE_ROWS = @TABLE_ROWS@;
    static const int TRANS_TABLE[TABLE_ROWS][128];
    
    static const int ACCEPT_RULES[TABLE_ROWS];
    
    static const int START_STATE = @START_STATE@;
};

} // namespace @NAMESPACE@
)";

  // --- SOURCE TEMPLATE (Maximal Munch) ---
  std::string source = R"(
#include "@CLASS_NAME@.hpp"
#include <cctype>

namespace @NAMESPACE@ {

// --- STATIC DATA ---
const int @CLASS_NAME@::TRANS_TABLE[@TABLE_ROWS@][128] = @TABLE_DATA@;

const int @CLASS_NAME@::ACCEPT_RULES[@TABLE_ROWS@] = @ACCEPT_DATA@;

// --- IMPLEMENTATION ---
@CLASS_NAME@::@CLASS_NAME@(std::istream& in) : input(in) {}

bool @CLASS_NAME@::hasMore() const {
    return input.good() && input.peek() != EOF;
}

int @CLASS_NAME@::nextToken(std::string& outLexeme) {
    outLexeme.clear();
    
    // 1. Skip Whitespace
    while (hasMore() && std::isspace(input.peek())) {
        char ch = input.peek();
        if(ch == '\n') { currentCol = 1; currentLine++; }
        else if (ch == '\t') { currentCol += (4 - ((currentCol - 1) % 4)); }
        else currentCol++;
        input.get(); 
    }

    if (!hasMore()) return -1; // EOF
    
    // CAPTURE START BOUNDARY HERE!
    // This is the exact column where the token text actually begins.
    int tokCol = currentCol;

    int currentState = START_STATE;
    std::string buffer;
    
    // Tracking for Maximal Munch (Longest Match)
    int lastGoodRule = -1;
    size_t lastGoodLength = 0;

    // 2. Driver Loop (Greedy Consumption)
    while (hasMore()) {
        char c = input.peek();
        
        // Ensure char is within 0-127
        if (c < 0 || c > 127) break; 

        int nextState = TRANS_TABLE[currentState][(int)c];

        if (nextState == -1) {
            // No transition possible. Stop reading.
            break;
        }

        // Consume and Advance
        buffer += c;
        input.get();
        currentState = nextState;
        
        // Is this new state accepting?
        if (ACCEPT_RULES[currentState] != -1) {
            lastGoodRule = ACCEPT_RULES[currentState];
            lastGoodLength = buffer.length();
        }
    }

    // 3. Final Decision
    if (lastGoodRule != -1) {
        // We found a match!
        outLexeme = buffer.substr(0, lastGoodLength);
        
        // ROLLBACK: Put back any characters we read that weren't part of the match
        // e.g. Input "intx", matched "int", read "intx". Put back 'x'.
        for (size_t i = buffer.length(); i > lastGoodLength; --i) {
            input.putback(buffer[i-1]);
        }

        tokenStartCol = tokCol;

        currentCol += outLexeme.length();
        
        @ACTION_SWITCH@

        return lastGoodRule;
    } else {
        // Error: We read some characters but never hit an accepting state
        if (!buffer.empty()) {
            // 1. Only harvest the VERY FIRST character as the actual error lexeme
            outLexeme = buffer.substr(0, 1);
            
            // 2. ROLLBACK: Put back all subsequent over-read characters 
            // so they can be parsed cleanly on the next nextToken() cycles!
            for (size_t i = buffer.length(); i > 1; --i) {
                input.putback(buffer[i-1]);
            }
            
            // 3. Lock in the error's exact starting position for diagnostics
            tokenStartCol = tokCol;
            
            // 4. Process coordinates precisely for this single invalid byte
            char ch = outLexeme[0];
            if(ch == '\n') { currentCol = 1; currentLine++; }
            else if(ch == '\t') { currentCol += (4 - ((currentCol - 1) % 4)); }
            else currentCol++;
            return -2; 
        }
        return -1; 
    }
}

} // namespace @NAMESPACE@

@USER_CODE@
)";

  auto processTemplate = [&](std::string text) -> std::string {
    replaceAll(text, "@NAMESPACE@", config.namespaceName);
    replaceAll(text, "@CLASS_NAME@", config.className);
    replaceAll(text, "@TABLE_ROWS@", rowsStr);
    replaceAll(text, "@TABLE_DATA@", tableData);
    replaceAll(text, "@ACCEPT_DATA@", acceptData);
    replaceAll(text, "@START_STATE@", startState);
    replaceAll(text, "@ACTION_SWITCH@", ss.str());
    replaceAll(text, "@HEADER_CODE@", spec.headerCode);
    replaceAll(text, "@USER_CODE@", spec.userCode);
    return text;
  };

  std::string headerContent = processTemplate(header);
  std::string sourceContent = processTemplate(source);

  writeToFile(config.className + ".hpp", headerContent);
  writeToFile(config.className + ".cpp", sourceContent);
}

} // namespace generator
} // namespace metalyzer
