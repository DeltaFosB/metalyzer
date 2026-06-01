#include "metalyzer_bench_harness.hpp"
#include <MyLexer.hpp>

static PassMetrics run_json_pass(std::ifstream &file_stream) {
  // Clear any EOF or fail bits from a previous pass and rewind to byte 0
  file_stream.clear();
  file_stream.seekg(0, std::ios::beg);

  // Directly instantiate the chunk-swapping lexer with the streaming data pipe
  user_code::MyLexer lexer(file_stream, false);

  std::string lexeme;
  lexeme.reserve(256); // Allocate out of bounds of hot iterations

  uint64_t token_count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  // Low-overhead execution loop driven by raw pointer buffer boundary checks
  while (lexer.hasMore()) {
    int tok = lexer.nextToken(lexeme);
    if (tok == -1) {
      break; // True EOF Boundary reached natively via internal chunk refresh
    }
    ++token_count;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  // Track file size using the stream's current ending position for MB/s
  // calculations
  file_stream.clear();
  file_stream.seekg(0, std::ios::end);
  double data_size_mb =
      static_cast<double>(file_stream.tellg()) / (1024.0 * 1024.0);

  PassMetrics metrics;
  metrics.elapsed_seconds = elapsed.count();
  metrics.total_tokens = token_count;
  metrics.mb_per_sec = data_size_mb / elapsed.count();
  metrics.tokens_per_sec = static_cast<double>(token_count) / elapsed.count();

  return metrics;
}

void worker_json_execution(const TargetPayload target, int target_core_id,
                           int display_row_offset) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(target_core_id, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  // Open the target handle as a persistent stream for this core's iterations
  std::ifstream file_stream(target.path, std::ios::binary);
  if (!file_stream.is_open()) {
    std::lock_guard<std::mutex> lock(console_mutex);
    std::cerr << "\n[Benchmark Error] JSON input file missing at "
              << target.path << "\n";
    return;
  }

  std::vector<char> local_eviction_buffer(CACHE_FLUSH_SIZE_BYTES, 0);
  std::vector<PassMetrics> history_pass1, history_pass2, history_pass3;
  history_pass1.reserve(STATISTICAL_ITERATIONS);
  history_pass2.reserve(STATISTICAL_ITERATIONS);
  history_pass3.reserve(STATISTICAL_ITERATIONS);

  for (int i = 0; i < STATISTICAL_ITERATIONS; ++i) {
    if (i % 5 == 0 || i == STATISTICAL_ITERATIONS - 1) {
      std::lock_guard<std::mutex> lock(console_mutex);
      int lines_to_move = global_active_threads - display_row_offset;
      std::cout << "\033[" << lines_to_move << "A\r\033[K"
                << "    -> [JSON_" << target.name << " on Core "
                << target_core_id << "] Progress: " << (i + 1) << " / "
                << STATISTICAL_ITERATIONS << " iterations..."
                << "\033[" << lines_to_move << "B" << std::flush;
    }

    // Flush hardware cache lines completely before running pass 1
    flush_hardware_caches(local_eviction_buffer);

    // Pass the active file stream descriptor down to the individual runs
    history_pass1.push_back(run_json_pass(file_stream));
    history_pass2.push_back(run_json_pass(file_stream));
    history_pass3.push_back(run_json_pass(file_stream));
  }

  {
    std::lock_guard<std::mutex> lock(console_mutex);
    int lines_to_move = global_active_threads - display_row_offset;
    std::cout << "\033[" << lines_to_move << "A\r\033[K"
              << "    -> [JSON_" << target.name << "] On Core "
              << target_core_id << " Finalized Cleanly."
              << "\033[" << lines_to_move << "B" << std::flush;
  }

  file_stream.close();

  AggregatedMetrics agg1 = compute_statistical_aggregates(history_pass1);
  AggregatedMetrics agg2 = compute_statistical_aggregates(history_pass2);
  AggregatedMetrics agg3 = compute_statistical_aggregates(history_pass3);

  double cache_delta_pct =
      ((agg2.mean_mb_per_sec - agg1.mean_mb_per_sec) / agg1.mean_mb_per_sec) *
      100.0;
  double stability_delta_pct =
      ((agg3.mean_mb_per_sec - agg2.mean_mb_per_sec) / agg2.mean_mb_per_sec) *
      100.0;

  ThreadResult record{"JSON_Core_" + target.name,
                      target_core_id,
                      agg1,
                      agg2,
                      agg3,
                      cache_delta_pct,
                      stability_delta_pct};

  std::lock_guard<std::mutex> result_lock(results_mutex);
  final_json_records.push_back(record);
}
