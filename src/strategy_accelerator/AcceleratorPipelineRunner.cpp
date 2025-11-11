#include "AcceleratorPipelineRunner.hpp"

#include "../../include/ff_includes.hpp"
#include "../common/StatsCollector.hpp"
#include "../ff_Pipe_nodes/Emitter.hpp"
#include "../ff_Pipe_nodes/ff_node_acc_t.hpp"

#include <chrono>
#include <future>
#include <iostream>

/**
 * @brief Costruttore. Prende possesso del puntatore all'acceleratore.
 */
AcceleratorPipelineRunner::AcceleratorPipelineRunner(std::unique_ptr<IAccelerator> accelerator)
    : accelerator_(std::move(accelerator)) {}

/**
 * @brief Orchestra l'intera pipeline FastFlow per l'offloading su un acceleratore. Crea i due
 * nodi della pipeline FF (Emitter, ff_node_acc_t). Riceve l'acceleratore già inizializzato.
 * Avvia la pipeline. Misura e raccoglie i tempi di esecuzione e il numero di task completati.
 */
ComputeResult AcceleratorPipelineRunner::execute(size_t N, size_t NUM_TASKS) {
   // Dati per ottenere il conteggio finale dei task processati.
   StatsCollector stats;
   std::future<size_t> count_future = stats.count_promise.get_future();

   // Creazione della pipeline FF e dei suoi due nodi (Emitter, ff_node_acc_t),
   // il cui secondo nodo incapsula una pipeline interna a 2 thread (producer, consumer).
   Emitter emitter(N, NUM_TASKS);
   ff_node_acc_t accNode(accelerator_.get(), &stats);
   ff_Pipe<> pipe(&emitter, &accNode);

   std::cout << "[Main] Starting FF pipeline execution...\n";
   auto t0 = std::chrono::steady_clock::now();

   // Avvio della pipeline e attesa del completamento.
   if (pipe.run_and_wait_end() < 0) {
      std::cerr << "[ERROR] Main: Pipeline execution failed.\n";
      exit(EXIT_FAILURE);
   }

   auto t1 = std::chrono::steady_clock::now();
   std::cout << "[Main] FF Pipeline execution finished.\n";

   // Raccolta dei risultati.
   ComputeResult res;
   res.tasks_completed = count_future.get();
   res.elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
   res.computed_ns = stats.computed_ns.load();
   res.total_InNode_time_ns = stats.total_InNode_time_ns.load();
   res.inter_completion_time_ns = stats.inter_completion_time_ns.load();

   return res;
}