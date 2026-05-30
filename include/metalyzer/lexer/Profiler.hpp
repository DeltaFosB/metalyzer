#pragma once

#include <cstdint>
#include <string>

namespace metalyzer {
namespace lexer {

struct ProfilingMetrics {
  uint32_t nfa_states = 0;
  uint32_t dfa_initial_states = 0;
  uint32_t dfa_minimized_states = 0;
  uint32_t alphabet_columns = 0;
  uint32_t type_size_bytes = 0;

  double getExplosionRatio() const {
    if (nfa_states == 0)
      return 0.0;
    return static_cast<double>(dfa_initial_states) / nfa_states;
  }

  double getReductionEfficiency() const {
    if (dfa_initial_states == 0)
      return 0.0;
    return (1.0 -
            (static_cast<double>(dfa_minimized_states) / dfa_initial_states)) *
           100.0;
  }

  uint64_t getMatrixFootprint() const {
    return static_cast<uint64_t>(dfa_minimized_states) * alphabet_columns *
           type_size_bytes;
  }

  std::string getCacheClaimString() const {
    uint64_t footprint = getMatrixFootprint();
    if (footprint <= 32768) {
      return "— fits within L1 data cache (32KB boundary)";
    } else {
      return "— transition table — L2 cache resident";
    }
  }

  std::string getCacheTier() const {
    return (getMatrixFootprint() <= 32768) ? "L1" : "L2";
  }
};

// Kept your exact function signature, adding the target output folder hook
void printProfilingReport(const std::string &spec_name,
                          const ProfilingMetrics &metrics,
                          const std::string &output_dir = "generated_lexer");

} // namespace lexer
} // namespace metalyzer
