#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <metalyzer/lexer/Profiler.hpp>

namespace metalyzer {
namespace lexer {

void printProfilingReport(const std::string &spec_name,
                          const ProfilingMetrics &metrics,
                          const std::string &output_dir) {

  std::filesystem::path dirPath(output_dir);
  if (!std::filesystem::exists(dirPath)) {
    std::filesystem::create_directories(dirPath);
  }

  std::filesystem::path filePath = dirPath / (spec_name + "_profile.json");
  std::ofstream jsonFile(filePath);

  if (jsonFile.is_open()) {
    jsonFile
        << "{\n"
        << "  \"grammar\": \"" << spec_name << "\",\n"
        << "  \"raw_measurements\": {\n"
        << "    \"nfa_states\": " << metrics.nfa_states << ",\n"
        << "    \"dfa_initial_states\": " << metrics.dfa_initial_states << ",\n"
        << "    \"dfa_minimized_states\": " << metrics.dfa_minimized_states
        << ",\n"
        << "    \"alphabet_columns\": " << metrics.alphabet_columns << ",\n"
        << "    \"type_bytes\": " << metrics.type_size_bytes << "\n"
        << "  },\n"
        << "  \"derived_metrics\": {\n"
        << "    \"explosion_ratio\": " << std::fixed << std::setprecision(2)
        << metrics.getExplosionRatio() << ",\n"
        << "    \"reduction_efficiency_pct\": " << std::fixed
        << std::setprecision(1) << metrics.getReductionEfficiency() << ",\n"
        << "    \"matrix_footprint_bytes\": " << metrics.getMatrixFootprint()
        << ",\n"
        << "    \"cache_tier\": \"" << metrics.getCacheTier() << "\"\n"
        << "  },\n"
        << "  \"interpretation\": {\n"
        << "    \"explosion\": \"" << std::fixed << std::setprecision(2)
        << metrics.getExplosionRatio()
        << "x NFA-to-DFA expansion via subset construction\",\n"
        << "    \"reduction\": \"" << std::fixed << std::setprecision(1)
        << metrics.getReductionEfficiency()
        << "% of redundant DFA states eliminated by Hopcroft partitioning\",\n"
        << "    \"footprint\": \"Flat " << metrics.getMatrixFootprint()
        << " B transition matrix " << metrics.getCacheClaimString() << "\"\n"
        << "  }\n"
        << "}\n";
    jsonFile.close();
  } else {
    std::cerr << "[Profiler Error] Could not write metadata payload to target: "
              << filePath << "\n";
  }

  // =========================================================================
  // TASK 2: REFINED HUMAN-READABLE CONSOLE REPORTING
  // =========================================================================
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
  std::cout << "  Minimization Efficiency   : " << std::fixed
            << std::setprecision(1) << metrics.getReductionEfficiency()
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
