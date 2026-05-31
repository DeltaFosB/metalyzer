#include "flex_bench_harness.hpp"
#include <thread>

// Global storage linking state arrays across translation regions
std::mutex console_mutex;
std::mutex results_mutex;
int global_active_threads = 0;
std::vector<ThreadResult> final_json_records;

// Forward link to isolated Flex engines compiled in separate translation units
void worker_flex_calculator_execution(const TargetPayload target,
                                      int target_core_id,
                                      int display_row_offset);
void worker_flex_json_execution(const TargetPayload target, int target_core_id,
                                int display_row_offset);
void worker_flex_c_subset_execution(const TargetPayload target,
                                    int target_core_id, int display_row_offset);

void write_json_output(const std::string &filename) {
  std::ofstream json_file(filename);
  json_file << "{\n  \"engine\": \"Flex\",\n  \"iterations\": "
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
  // Structural layout mapping the baseline 9-point target vectors for Flex
  std::vector<TargetPayload> calc_payloads = {
      {"DENSE_CODE", "../benchmarks/data/calc_dense.txt"},
      {"SPARSE_SPACES", "../benchmarks/data/calc_sparse.txt"},
      {"ERROR_CHURN", "../benchmarks/data/calc_error.txt"}};

  std::vector<TargetPayload> json_payloads = {
      {"DENSE_CODE", "../benchmarks/data/json_dense.txt"},
      {"SPARSE_SPACES", "../benchmarks/data/json_sparse.txt"},
      {"ERROR_CHURN", "../benchmarks/data/json_error.txt"}};

  std::vector<TargetPayload> c_sub_payloads = {
      {"DENSE_CODE", "../benchmarks/data/c_dense.txt"},
      {"SPARSE_SPACES", "../benchmarks/data/c_sparse.txt"},
      {"ERROR_CHURN", "../benchmarks/data/c_error.txt"}};

  std::cout << "==============================================================="
               "==================\n";
  std::cout
      << "INDUSTRY-STANDARD FLEX BATCHED MULTI-GRAMMAR THROUGHPUT LABORATORY\n";
  std::cout << "==============================================================="
               "==================\n";
  std::cout << "[*] System Architecture Detected: 4 Physical Cores / 8 Logical "
               "Threads\n";
  std::cout << "[*] Optimization Strategy: Isolating benchmarks to independent "
               "physical cores (CPU 0, 2, 4)\n";
  std::cout << "[*] Running 3 sequential batches to prevent hardware "
               "over-subscription...\n";

  // Create terminal row vertical display spacers
  for (int i = 0; i < 3; ++i) {
    std::cout << "\n";
  }

  // Define our 3 dedicated hardware physical core affinity coordinates
  // (skipping hyper-threaded siblings)
  int hardware_cores[3] = {0, 2, 4};

  // -------------------------------------------------------------------------
  // BATCH 1: Flex Calculator Grammar Sweep
  // -------------------------------------------------------------------------
  {
    global_active_threads = 3;
    std::vector<std::thread> batch_workers;
    batch_workers.reserve(3);

    for (int i = 0; i < 3; ++i) {
      batch_workers.push_back(std::thread(worker_flex_calculator_execution,
                                          calc_payloads[i], hardware_cores[i],
                                          i));
    }
    for (auto &worker : batch_workers) {
      if (worker.joinable())
        worker.join();
    }
  }

  // -------------------------------------------------------------------------
  // BATCH 2: Flex JSON Core Grammar Sweep
  // -------------------------------------------------------------------------
  {
    global_active_threads = 3;
    std::vector<std::thread> batch_workers;
    batch_workers.reserve(3);

    for (int i = 0; i < 3; ++i) {
      batch_workers.push_back(std::thread(
          worker_flex_json_execution, json_payloads[i], hardware_cores[i], i));
    }
    for (auto &worker : batch_workers) {
      if (worker.joinable())
        worker.join();
    }
  }

  // -------------------------------------------------------------------------
  // BATCH 3: Flex C Subset Grammar Sweep
  // -------------------------------------------------------------------------
  {
    global_active_threads = 3;
    std::vector<std::thread> batch_workers;
    batch_workers.reserve(3);

    for (int i = 0; i < 3; ++i) {
      batch_workers.push_back(std::thread(worker_flex_c_subset_execution,
                                          c_sub_payloads[i], hardware_cores[i],
                                          i));
    }
    for (auto &worker : batch_workers) {
      if (worker.joinable())
        worker.join();
    }
  }

  // Serialize aggregated, interference-free run metrics
  write_json_output("flex_metrics.json");

  std::cout << "\n[+] Data acquisition completed. Industry baseline metrics "
               "serialized to 'flex_metrics.json'\n";
  std::cout << "==============================================================="
               "==================\n";
  return 0;
}
