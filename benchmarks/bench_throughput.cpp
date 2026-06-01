#include <MyLexer.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Linux native scheduling headers for core affinity
#include <pthread.h>
#include <sched.h>

constexpr int STATISTICAL_ITERATIONS = 100; // Optimized sample scale
constexpr size_t CACHE_FLUSH_SIZE_BYTES = 32 * 1024 * 1024;

struct PassMetrics {
  double elapsed_seconds = 0.0;
  uint64_t total_tokens = 0;
  double mb_per_sec = 0.0;
  double tokens_per_sec = 0.0;
};

struct AggregatedMetrics {
  double mean_mb_per_sec = 0.0;
  double stddev_mb_per_sec = 0.0;
  double mean_tokens_per_sec = 0.0;
  double mean_seconds = 0.0;
  uint64_t total_tokens = 0;
};

struct TargetPayload {
  std::string name;
  std::string path;
};

struct ThreadResult {
  std::string name;
  int core_id;
  AggregatedMetrics pass1;
  AggregatedMetrics pass2;
  AggregatedMetrics pass3;
  double cache_delta_pct;
  double stability_delta_pct;
};

std::mutex console_mutex;
std::mutex results_mutex;
int global_active_threads = 0;
std::vector<ThreadResult> final_json_records;

class ZeroCopyReadOnlyBuf : public std::streambuf {
public:
  ZeroCopyReadOnlyBuf(const char *base, size_t size) {
    char *p = const_cast<char *>(base);
    setg(p, p, p + size);
  }
  size_t get_pos() const { return gptr() - eback(); }
};

void flush_hardware_caches(std::vector<char> &eviction_buffer) {
  volatile char *data = eviction_buffer.data();
  size_t size = eviction_buffer.size();
  for (size_t i = 0; i < size; i += 64) {
    data[i] = static_cast<char>(i & 0xFF);
  }
}

PassMetrics run_benchmark_pass(const std::string &input_buffer) {
  // 1. Instantiation: Pass the memory-resident payload directly to the
  // bare-metal lexer
  user_code::MyLexer lexer(input_buffer, false);

  std::string lexeme;
  lexeme.reserve(256);

  uint64_t token_count = 0;

  // Capture the starting timestamp marker right before entering the hot path
  auto start_time = std::chrono::high_resolution_clock::now();

  // 2. Hot Path: Low-overhead execution loop driven by raw pointer boundary
  // checks
  while (lexer.hasMore()) {
    int tok = lexer.nextToken(lexeme);
    if (tok == -1) {
      break; // Clear EOF hit boundary reached
    }
    ++token_count;
  }

  // Capture the termination timestamp marker
  auto end_time = std::chrono::high_resolution_clock::now();

  // 3. Metric Aggregation Vector Math
  std::chrono::duration<double> elapsed_seconds = end_time - start_time;
  double duration = elapsed_seconds.count();

  // Prevent floating-point division exceptions if the file is vanishingly small
  if (duration <= 0.0) {
    duration = 1e-9;
  }

  double total_mb =
      static_cast<double>(input_buffer.size()) / (1024.0 * 1024.0);
  double mb_per_sec = total_mb / duration;
  double tokens_per_sec = static_cast<double>(token_count) / duration;

  // Instantiate output dataset object structure
  PassMetrics agg;
  agg.mean_seconds = duration;
  agg.scanned_tokens = token_count;
  agg.mean_mb_per_sec = mb_per_sec;
  agg.mean_tokens_per_sec = tokens_per_sec;

  // 4. Statistical Variance Calculations (Tracking delta distributions against
  // historical loops)
  history.push_back(agg);
  size_t n = history.size();

  double total_mb_velocity_sum = 0.0;
  for (const auto &run : history) {
    total_mb_velocity_sum += run.mean_mb_per_sec;
  }
  agg.mean_mb_per_sec = total_mb_velocity_sum / n;

  double variance_sum = 0.0;
  for (const auto &run : history) {
    double diff = run.mean_mb_per_sec - agg.mean_mb_per_sec;
    variance_sum += diff * diff;
  }
  agg.stddev_mb_per_sec = std::sqrt(variance_sum / n);

  return agg;
}

void worker_core_execution(const TargetPayload target, int target_core_id,
                           int display_row_offset) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(target_core_id, &cpuset);
  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

  std::ifstream file_stream(target.path, std::ios::binary);
  if (!file_stream.is_open()) {
    std::lock_guard<std::mutex> lock(console_mutex);
    std::cerr << "\n[Benchmark Error] Input file missing at " << target.path
              << "\n";
    return;
  }

  std::stringstream buffer;
  buffer << file_stream.rdbuf();
  std::string flat_input_payload = buffer.str();
  file_stream.close();

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
                << "    -> [" << target.name << " on Core " << target_core_id
                << "] Progress: " << (i + 1) << " / " << STATISTICAL_ITERATIONS
                << " iterations..."
                << "\033[" << lines_to_move << "B" << std::flush;
    }

    flush_hardware_caches(local_eviction_buffer);
    history_pass1.push_back(run_benchmark_pass(flat_input_payload));
    history_pass2.push_back(run_benchmark_pass(flat_input_payload));
    history_pass3.push_back(run_benchmark_pass(flat_input_payload));
  }

  {
    std::lock_guard<std::mutex> lock(console_mutex);
    int lines_to_move = global_active_threads - display_row_offset;
    std::cout << "\033[" << lines_to_move << "A\r\033[K"
              << "    -> [" << target.name << "] On Core " << target_core_id
              << " Finalized Cleanly."
              << "\033[" << lines_to_move << "B" << std::flush;
  }

  AggregatedMetrics agg1 = compute_statistical_aggregates(history_pass1);
  AggregatedMetrics agg2 = compute_statistical_aggregates(history_pass2);
  AggregatedMetrics agg3 = compute_statistical_aggregates(history_pass3);

  double cache_delta_pct =
      ((agg2.mean_mb_per_sec - agg1.mean_mb_per_sec) / agg1.mean_mb_per_sec) *
      100.0;
  double stability_delta_pct =
      ((agg3.mean_mb_per_sec - agg2.mean_mb_per_sec) / agg2.mean_mb_per_sec) *
      100.0;

  // Cache record for structured file output safely
  ThreadResult record{target.name,     target_core_id,     agg1, agg2, agg3,
                      cache_delta_pct, stability_delta_pct};
  std::lock_guard<std::mutex> result_lock(results_mutex);
  final_json_records.push_back(record);
}

void write_json_output(const std::string &filename) {
  std::ofstream json_file(filename);
  json_file << "{\n  \"engine\": \"Metalyzer\",\n  \"iterations\": "
            << STATISTICAL_ITERATIONS << ",\n  \"profiles\": {\n";

  for (size_t i = 0; i < final_json_records.size(); ++i) {
    const auto &rec = final_json_records[i];
    json_file
        << "    \"" << rec.name << "\": {\n"
        << "      \"pinned_core\": " << rec.core_id << ",\n"
        << "      \"pass1_cold\": {\n"
        << "        \"mean_seconds\": " << rec.pass1.mean_seconds << ",\n"
        << "        \"scanned_tokens\": " << rec.pass1.total_tokens << ",\n"
        << "        \"mean_mb_per_sec\": " << rec.pass1.mean_mb_per_sec << ",\n"
        << "        \"stddev_mb_per_sec\": " << rec.pass1.stddev_mb_per_sec
        << ",\n"
        << "        \"mean_tokens_per_sec\": " << rec.pass1.mean_tokens_per_sec
        << "\n"
        << "      },\n"
        << "      \"pass2_warmed\": {\n"
        << "        \"mean_seconds\": " << rec.pass2.mean_seconds << ",\n"
        << "        \"mean_mb_per_sec\": " << rec.pass2.mean_mb_per_sec << ",\n"
        << "        \"stddev_mb_per_sec\": " << rec.pass2.stddev_mb_per_sec
        << ",\n"
        << "        \"mean_tokens_per_sec\": " << rec.pass2.mean_tokens_per_sec
        << "\n"
        << "      },\n"
        << "      \"pass3_stable\": {\n"
        << "        \"mean_seconds\": " << rec.pass3.mean_seconds << ",\n"
        << "        \"mean_mb_per_sec\": " << rec.pass3.mean_mb_per_sec << ",\n"
        << "        \"stddev_mb_per_sec\": " << rec.pass3.stddev_mb_per_sec
        << ",\n"
        << "        \"mean_tokens_per_sec\": " << rec.pass3.mean_tokens_per_sec
        << "\n"
        << "      },\n"
        << "      \"cache_warmup_acceleration_delta_pct\": "
        << rec.cache_delta_pct << ",\n"
        << "      \"steady_state_stability_variance_delta_pct\": "
        << rec.stability_delta_pct << "\n"
        << "    }" << (i == final_json_records.size() - 1 ? "" : ",") << "\n";
  }
  json_file << "  }\n}\n";
}

int main() {
  std::vector<TargetPayload> payloads = {
      {"DENSE_CODE", "../benchmarks/data/calc_dense.txt"},
      {"SPARSE_SPACES", "../benchmarks/data/calc_sparse.txt"},
      {"ERROR_CHURN", "../benchmarks/data/calc_error.txt"}};

  global_active_threads = static_cast<int>(payloads.size());

  std::cout << "==============================================================="
               "==================\n";
  std::cout
      << "METALYZER MULTI-CORE CACHE-ISOLATED PARALLEL THROUGHPUT LABORATORY\n";
  std::cout << "==============================================================="
               "==================\n";
  std::cout << "[*] Spawning hardware-isolated loops. Processing 200 "
               "iterations concurrently...\n";

  for (int i = 0; i < global_active_threads; ++i) {
    std::cout << "\n";
  }

  std::vector<std::thread> workers;
  workers.reserve(payloads.size());

  for (int i = 0; i < global_active_threads; ++i) {
    int target_core_id = 1;
    if (payloads[i].name == "DENSE_CODE")
      target_core_id = 1;
    else if (payloads[i].name == "SPARSE_SPACES")
      target_core_id = 2;
    else if (payloads[i].name == "ERROR_CHURN")
      target_core_id = 3;

    workers.push_back(
        std::thread(worker_core_execution, payloads[i], target_core_id, i));
  }

  for (auto &worker : workers) {
    if (worker.joinable())
      worker.join();
  }

  write_json_output("metalyzer_metrics.json");
  std::cout << "\n[+] Data acquisition completed. Results cleanly serialized "
               "to 'metalyzer_metrics.json'\n";
  std::cout << "==============================================================="
               "==================\n";
  return 0;
}
