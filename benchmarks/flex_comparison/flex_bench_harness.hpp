#pragma once
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <streambuf>
#include <string>
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
  std::string name; // Mapped as "GrammarName_ProfileType" (e.g.
                    // "Flex_Calculator_DENSE_CODE")
  int core_id;
  AggregatedMetrics pass1;
  AggregatedMetrics pass2;
  AggregatedMetrics pass3;
  double cache_delta_pct;
  double stability_delta_pct;
};

// Share tracking states securely across separate compilation units
extern std::mutex console_mutex;
extern std::mutex results_mutex;
extern int global_active_threads;
extern std::vector<ThreadResult> final_json_records;

class ZeroCopyReadOnlyBuf : public std::streambuf {
public:
  ZeroCopyReadOnlyBuf(const char *base, size_t size) {
    char *p = const_cast<char *>(base);
    setg(p, p, p + size);
  }
};

inline void flush_hardware_caches(std::vector<char> &eviction_buffer) {
  volatile char *data = eviction_buffer.data();
  size_t size = eviction_buffer.size();
  for (size_t i = 0; i < size; i += 64) {
    data[i] = static_cast<char>(i & 0xFF);
  }
}

inline AggregatedMetrics
compute_statistical_aggregates(const std::vector<PassMetrics> &history) {
  AggregatedMetrics agg;
  if (history.empty())
    return agg;

  size_t n = history.size();
  agg.total_tokens = history[0].total_tokens;

  double sum_mb = 0.0;
  double sum_tok_sec = 0.0;
  double sum_sec = 0.0;

  for (const auto &run : history) {
    sum_mb += run.mb_per_sec;
    sum_tok_sec += run.tokens_per_sec;
    sum_sec += run.elapsed_seconds;
  }

  agg.mean_mb_per_sec = sum_mb / n;
  agg.mean_tokens_per_sec = sum_tok_sec / n;
  agg.mean_seconds = sum_sec / n;

  double variance_sum = 0.0;
  for (const auto &run : history) {
    double diff = run.mb_per_sec - agg.mean_mb_per_sec;
    variance_sum += diff * diff;
  }
  agg.stddev_mb_per_sec = std::sqrt(variance_sum / n);

  return agg;
}
