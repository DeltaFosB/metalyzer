#pragma once

#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/TransTable.hpp>

namespace metalyzer {
namespace lexer {

class Compressor {
public:
  TransTable compress(const DFA &dfa);
};
} // namespace lexer
} // namespace metalyzer
