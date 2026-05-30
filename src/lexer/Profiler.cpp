#include <iomanip>
#include <iostream>
#include <metalyzer/lexer/Profiler.hpp>

namespace metalyzer {
namespace lexer {

void printProfilingReport(const std::string &spec_name,
                          const ProfilingMetrics &metrics) {
  std::cout << "\n======================= METALYZER COMPILER PROFILER "
               "=======================\n";
  std::cout << "Target Specification      : " << spec_name << "\n\n";

  std::cout << "[RAW GRAPH MEASUREMENTS]\n";
  std::cout << "  NFA State Count           : " << metrics.nfa_states
            << " states (True Thompson construction including epsilon nodes)\n";
  std::cout << "  DFA Initial State Count   : " << metrics.dfa_initial_states
            << " states (Raw power-set expansion before optimization)\n";
  std::cout << "  DFA Minimized State Count : " << metrics.dfa_minimized_states
            << " states (Final deterministic state nodes retained)\n\n";

  std::cout << "[RELATIONAL PERFORMANCE ANALYTICS]\n";
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "  DFA State-Explosion Ratio : " << metrics.getExplosionRatio()
            << "x state expansion during Subset Construction phase\n";
  std::cout << "  Minimization Efficiency   : "
            << metrics.getReductionEfficiency()
            << "% of bloated intermediate states reclaimed via Hopcroft "
               "partitioning\n\n";

  std::cout << "[HARDWARE MEMORY ALLOCATION STRUCT]\n";
  std::cout << "  Matrix Cache Footprint    : " << metrics.getMatrixFootprint()
            << " B (" << metrics.dfa_minimized_states << " states x "
            << metrics.alphabet_columns << " cols x " << metrics.type_size_bytes
            << " bytes) " << metrics.getCacheClaimString() << "\n";
  std::cout << "==============================================================="
               "============\n\n";
}

} // namespace lexer
} // namespace metalyzer
