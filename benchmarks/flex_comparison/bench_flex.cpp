#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <FlexLexer.h>

struct PassMetrics {
  double elapsed_seconds = 0.0;
  uint64_t total_tokens = 0;
  double mb_per_sec = 0.0;
  double tokens_per_sec = 0.0;
};

PassMetrics run_flex_pass(const std::string &input_buffer) {
  // Standard stringstream allows Flex's C++ core to recognize EOF boundary
  // marks natively
  std::istringstream stream(input_buffer);

  // Pass the stream reference directly into the FlexLexer constructor
  yyFlexLexer lexer(&stream);

  uint64_t token_count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  while (true) {
    int tok = lexer.yylex();
    // Flex returns 0 when the underlying C++ istream hits EOF natively
    if (tok == 0) {
      break;
    }
    if (tok == -2) { // TOK_ERROR breakdown safety
      break;
    }
    ++token_count;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  double data_size_mb =
      static_cast<double>(input_buffer.size()) / (1024.0 * 1024.0);

  PassMetrics metrics;
  metrics.elapsed_seconds = elapsed.count();
  metrics.total_tokens = token_count;
  metrics.mb_per_sec = data_size_mb / elapsed.count();
  metrics.tokens_per_sec = static_cast<double>(token_count) / elapsed.count();

  return metrics;
}

int main() {
  std::string input_path = "../tests/inputs/bench_calculator_input.txt";
  std::ifstream file_stream(input_path, std::ios::binary);

  if (!file_stream.is_open()) {
    std::cerr << "[Flex Benchmark Error] High-volume input file missing at "
              << input_path << "\n";
    return 1;
  }

  std::stringstream buffer;
  buffer << file_stream.rdbuf();
  std::string flat_input_payload = buffer.str();
  file_stream.close();

  double data_size_mb =
      static_cast<double>(flat_input_payload.size()) / (1024.0 * 1024.0);

  std::cout << "==============================================================="
               "==========\n";
  std::cout << "INDUSTRY-STANDARD FLEX C++ THROUGHPUT HARDWARE LABORATORY\n";
  std::cout << "==============================================================="
               "==========\n";
  std::cout << "Payload Weight:         " << std::fixed << std::setprecision(2)
            << data_size_mb << " MB\n\n";

  std::cout << "[*] Executing Cold Pass Sweep..." << std::flush;
  PassMetrics cold_pass = run_flex_pass(flat_input_payload);
  std::cout << " Done.\n";

  std::cout << "[*] Executing Warm Pass Sweep..." << std::flush;
  PassMetrics warm_pass = run_flex_pass(flat_input_payload);
  std::cout << " Done.\n\n";

  double cache_delta_pct =
      ((warm_pass.mb_per_sec - cold_pass.mb_per_sec) / cold_pass.mb_per_sec) *
      100.0;

  std::cout << "[FLEX PERFORMANCE RESULT MATRIX]\n";
  std::cout << "---------------------------------------------------------------"
               "----------\n";
  std::cout << " METRIC                     | COLD PASS          | WARM PASS   "
               "       \n";
  std::cout << "---------------------------------------------------------------"
               "----------\n";
  std::cout << " Time Elapsed (seconds)     | " << std::left << std::setw(18)
            << std::setprecision(6) << cold_pass.elapsed_seconds << " | "
            << std::left << std::setw(18) << warm_pass.elapsed_seconds << "\n";
  std::cout << " Total Scanned Tokens       | " << std::left << std::setw(18)
            << cold_pass.total_tokens << " | " << std::left << std::setw(18)
            << warm_pass.total_tokens << "\n";
  std::cout << " Processing Velocity        | " << std::left << std::setw(10)
            << std::setprecision(2) << cold_pass.mb_per_sec << " MB/sec | "
            << std::left << std::setw(10) << warm_pass.mb_per_sec
            << " MB/sec\n";
  std::cout << " Token Density Throughput   | " << std::left << std::setw(10)
            << std::scientific << cold_pass.tokens_per_sec << " tok/s | "
            << std::left << std::setw(10) << warm_pass.tokens_per_sec
            << " tok/s\n";
  std::cout << "---------------------------------------------------------------"
               "----------\n";

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Derived Cache-Warmup Acceleration Delta: " << cache_delta_pct
            << "%\n";
  std::cout << "==============================================================="
               "==========\n\n";

  return 0;
}
