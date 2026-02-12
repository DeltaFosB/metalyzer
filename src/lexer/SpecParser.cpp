#include <algorithm>
#include <iostream>
#include <metalyzer/frontend/SpecParser.hpp>
#include <sstream>
#include <stdexcept>

namespace metalyzer {
namespace frontend {

LexerSpec SpecParser::parse(const std::string &content) {
  LexerSpec spec;

  size_t firstSep = content.find("%%");
  if (firstSep == std::string::npos) {
    throw std::runtime_error(
        "Syntax Error: Missing first '%%' section separator.");
  }

  size_t secondSep = content.find("%%", firstSep + 2);

  parseDefinitions(content.substr(0, firstSep), spec);

  std::string rulesSection;
  if (secondSep == std::string::npos) {
    rulesSection = content.substr(firstSep + 2);
  } else {
    rulesSection = content.substr(firstSep + 2, secondSep - (firstSep + 2));
    parseUserCode(content.substr(secondSep + 2), spec);
  }

  parseRules(rulesSection, spec);

  return spec;
}

void SpecParser::parseDefinitions(const std::string &section, LexerSpec &spec) {
  size_t start = section.find("%{");
  size_t end = section.find("%}");

  if (start != std::string::npos && end != std::string::npos) {
    spec.headerCode = section.substr(start + 2, end - (start + 2));
  } else {
    spec.headerCode = "";
  }
  spec.headerCode = trim(spec.headerCode);
}

void SpecParser::parseRules(const std::string &section, LexerSpec &spec) {
  std::stringstream ss(section);
  std::string line;
  int priorityCounter = 0;

  while (std::getline(ss, line)) {
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty())
      continue;
    if (trimmedLine.rfind("//", 0) == 0)
      continue;

    // Parse: REGEX [whitespace] ACTION
    // Example: [0-9]+    { return 1; }

    size_t splitPos = 0;
    bool foundSplit = false;

    for (size_t i = 0; i < trimmedLine.length(); ++i) {
      if (isspace(trimmedLine[i])) {
        splitPos = i;
        foundSplit = true;
        break;
      }
    }

    if (!foundSplit) {
      std::cerr << "Warning: Skipping invalid rule line: " << trimmedLine
                << std::endl;
      continue;
    }

    std::string regex = trimmedLine.substr(0, splitPos);
    std::string action = trimmedLine.substr(splitPos);

    action = trim(action);

    LexerRule rule;
    rule.regex = regex;
    rule.actionCode = action;
    rule.priority = priorityCounter++;

    spec.rules.push_back(rule);
  }
}

void SpecParser::parseUserCode(const std::string &section, LexerSpec &spec) {
  spec.userCode = trim(section);
}

std::string SpecParser::trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
}

} // namespace frontend
} // namespace metalyzer
