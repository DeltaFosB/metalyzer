#pragma once

#include <metalyzer/lexer/DFA.hpp>
#include <metalyzer/lexer/TransTable.hpp>

namespace metalyzer {

class Compressor {
public:
  TransTable compress(const DFA &dfa);
};

} // namespace metalyzer
