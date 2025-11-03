#pragma once

#include <atomic>
#include <future>

/**
 * @brief Struttura usata per raccogliere risultati generati dai 2 thread interni del nodo
 * accelerato FF e passarli al thread principale in AcceleratorPipelineStrategy.
 */
struct StatsCollector {
   std::atomic<size_t> tasks_processed{0};
   std::promise<size_t> count_promise;
   std::atomic<long long> computed_ns{0};
   std::atomic<long long> total_InNode_time_ns{0};
   std::atomic<long long> inter_completion_time_ns{0};
};