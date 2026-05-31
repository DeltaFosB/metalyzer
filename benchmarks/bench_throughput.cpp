#include <MyLexer.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

struct PassMetrics {
  double elapsed_seconds = 0.0;
  uint64_t total_tokens = 0;
  double mb_per_sec = 0.0;
  double tokens_per_sec = 0.0;
};

// Zero-overhead native stream buffer that wraps raw pointers directly
class ZeroCopyReadOnlyBuf : public std::streambuf {
public:
  ZeroCopyReadOnlyBuf(const char *base, size_t size) {
    char *p = const_cast<char *>(base);
    setg(p, p, p + size); // Pins reading boundaries directly to our contiguous
                          // string memory
  }
};

PassMetrics run_benchmark_pass(const std::string &input_buffer) {
  // Wrap the string memory directly without creating heap string copies
  ZeroCopyReadOnlyBuf raw_buffer(input_buffer.data(), input_buffer.size());
  std::istream stream(&raw_buffer);

  user_code::MyLexer lexer(stream, false);
  std::string lexeme;

  // Maximize string buffer capacity upfront to eliminate char-by-char resizes
  // inside nextToken
  lexeme.reserve(256);

  uint64_t token_count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  while (lexer.hasMore()) {
    int tok = lexer.nextToken(lexeme);
    if (tok == -1)
      break;
    if (tok == -2)
      break;
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
    std::cerr << "[Benchmark Error] High-volume input file missing at "
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
  std::cout << "METALYZER OPTIMIZED ZERO-COPY THROUGHPUT HARDWARE LABORATORY\n";
  std::cout << "==============================================================="
               "==========\n";
  std::cout << "Payload Weight:         " << std::fixed << std::setprecision(2)
            << data_size_mb << " MB\n\n";

  std::cout << "[*] Executing Pass 1 (Cold)..." << std::flush;
  PassMetrics pass1 = run_benchmark_pass(flat_input_payload);
  std::cout << " Done.\n";

  std::cout << "[*] Executing Pass 2 (Warm)..." << std::flush;
  PassMetrics pass2 = run_benchmark_pass(flat_input_payload);
  std::cout << " Done.\n";

  std::cout << "[*] Executing Pass 3 (Warm)..." << std::flush;
  PassMetrics pass3 = run_benchmark_pass(flat_input_payload);
  std::cout << " Done.\n\n";

  double cache_delta_pct =
      ((pass2.mb_per_sec - pass1.mb_per_sec) / pass1.mb_per_sec) * 100.0;

  double stability_delta_pct =
      ((pass3.mb_per_sec - pass2.mb_per_sec) / pass2.mb_per_sec) * 100.0;

  std::cout << "[PERFORMANCE RESULT MATRIX]\n";
  std::cout << "---------------------------------------------------------------"
               "------------------\n";
  std::cout << " METRIC                     | PASS 1 (COLD)      | PASS 2 "
               "(WARM)      | PASS 3 (WARM)\n";
  std::cout << "---------------------------------------------------------------"
               "------------------\n";
  std::cout << " Time Elapsed (seconds)     | " << std::left << std::setw(18)
            << std::setprecision(6) << pass1.elapsed_seconds << " | "
            << std::left << std::setw(18) << pass2.elapsed_seconds << " | "
            << std::left << std::setw(18) << pass3.elapsed_seconds << "\n";
  std::cout << " Total Scanned Tokens       | " << std::left << std::setw(18)
            << pass1.total_tokens << " | " << std::left << std::setw(18)
            << pass2.total_tokens << " | " << std::left << std::setw(18)
            << pass3.total_tokens << "\n";
  std::cout << " Processing Velocity        | " << std::left << std::setw(10)
            << std::setprecision(2) << pass1.mb_per_sec << " MB/sec | "
            << std::left << std::setw(10) << pass2.mb_per_sec << " MB/sec | "
            << std::left << std::setw(10) << pass3.mb_per_sec << " MB/sec\n";
  std::cout << " Token Density Throughput   | " << std::left << std::setw(10)
            << std::scientific << pass1.tokens_per_sec << " tok/s | "
            << std::left << std::setw(10) << pass2.tokens_per_sec << " tok/s | "
            << std::left << std::setw(10) << pass3.tokens_per_sec << " tok/s\n";
  std::cout << "---------------------------------------------------------------"
               "------------------\n";

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Derived Cache-Warmup Acceleration Delta (Pass 1 -> Pass 2): "
            << cache_delta_pct << "%\n";
  std::cout << "Warm Run Stability Deviation Delta (Pass 2 -> Pass 3):      "
            << stability_delta_pct << "%\n";
  std::cout << "==============================================================="
               "==================\n\n";

  return 0;
}
