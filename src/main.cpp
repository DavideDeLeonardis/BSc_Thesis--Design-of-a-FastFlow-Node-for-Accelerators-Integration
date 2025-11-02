#include "../../include/ff_includes.hpp"
#include "cpu_runner/Cpu_FF_Runner.hpp"
#include "ff_Pipe_nodes/Emitter.hpp"
#include "ff_Pipe_nodes/ff_node_acc_t.hpp"
#include "helpers/Helpers.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef __APPLE__
#include "accelerator/Gpu_Metal_Accelerator.hpp"
#include "accelerator/Gpu_OpenCL_Accelerator.hpp"
#else
#include "accelerator/FpgaAccelerator.hpp"
#include "cpu_runner/Cpu_OMP_Runner.hpp"
#endif

/**
 * @brief Orchestra l'intera pipeline FastFlow per l'offloading su un
 * acceleratore. Crea i due nodi della pipeline FF (Emitter, ff_node_acc_t).
 * Riceve l'acceleratore già inizializzato. Avvia la pipeline. Misura e
 * raccoglie i tempi di esecuzione e il numero di task completati.
 */
void runAcceleratorPipeline(size_t N, size_t NUM_TASKS, IAccelerator *accelerator,
                            long long &elapsed_ns, long long &computed_ns,
                            long long &total_InNode_time_ns, long long &inter_completion_time_ns,
                            size_t &final_count) {

   // Dati per ottenere il conteggio finale dei task processati.
   StatsCollector stats;
   std::future<size_t> count_future = stats.count_promise.get_future();

   // Creazione della pipeline FF e dei suoi due nodi (Emitter, ff_node_acc_t),
   // il cui secondo nodo incapsula una pipeline interna a 2 thread (producer,
   // consumer).
   Emitter emitter(N, NUM_TASKS);
   ff_node_acc_t accNode(accelerator, &stats);
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
   final_count = count_future.get();
   elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
   computed_ns = stats.computed_ns.load();
   total_InNode_time_ns = stats.total_InNode_time_ns.load();
   inter_completion_time_ns = stats.inter_completion_time_ns.load();
}

int main(int argc, char *argv[]) {
   // Parametri della command line.
   size_t N = 1000000, NUM_TASKS = 20; // Default
   std::string device_type = "cpu_ff"; // Default: cpu che usa ff::parallel_for
   std::string kernel_path, kernel_name;

   long long elapsed_ns = 0;           // Tempo totale (host) per completare tutti i task
   long long computed_ns = 0;          // Tempo per il singolo calcolo
   size_t final_count = 0;             // Numero totale di task effettivamente completati
   long long total_InNode_time_ns = 0; // Tempo totale dei task dall'ingresso all'uscita del nodo
   long long inter_completion_time_ns = 0; // Tempo di completamento fra due task consecutivi

   // Parsing degli argomenti della command line. Setta anche il kernel di
   // default per GPU e FPGA.
   parse_args(argc, argv, N, NUM_TASKS, device_type, kernel_path, kernel_name);

   print_configuration(N, NUM_TASKS, device_type, kernel_path, kernel_name);

   // In base al device scelto, esegue la parallelizzazione dei task su CPU
   // multicore tramite ff o la pipeline con offloading su GPU/FPGA.
   if (device_type == "cpu_ff")
      elapsed_ns = executeCpu_FF_Tasks(N, NUM_TASKS, kernel_name, final_count);

#ifdef __APPLE__
   else if (device_type == "gpu_opencl") {
      auto accelerator = std::make_unique<Gpu_OpenCL_Accelerator>(kernel_path, kernel_name);
      runAcceleratorPipeline(N, NUM_TASKS, accelerator.get(), elapsed_ns, computed_ns,
                             total_InNode_time_ns, inter_completion_time_ns, final_count);

   } else if (device_type == "gpu_metal") {
      auto accelerator = std::make_unique<Gpu_Metal_Accelerator>(kernel_path, kernel_name);
      runAcceleratorPipeline(N, NUM_TASKS, accelerator.get(), elapsed_ns, computed_ns,
                             total_InNode_time_ns, inter_completion_time_ns, final_count);
   }
#else
   else if (device_type == "cpu_omp") {
      elapsed_ns = executeCpu_OMP_Tasks(N, NUM_TASKS, kernel_name, final_count);

   } else if (device_type == "fpga") {
      auto accelerator = std::make_unique<FpgaAccelerator>(kernel_path, kernel_name);
      runAcceleratorPipeline(N, NUM_TASKS, accelerator.get(), elapsed_ns, computed_ns,
                             total_InNode_time_ns, inter_completion_time_ns, final_count);
   }
#endif
   else {
      std::cerr << "[ERROR] Invalid device type '" << device_type << "' for this OS.\n\n";
      print_usage(argv[0]);
      return -1;
   }

   PerformanceData metrics = calculate_metrics(elapsed_ns, computed_ns, total_InNode_time_ns,
                                               inter_completion_time_ns, final_count);
   print_metrics(N, NUM_TASKS, device_type, kernel_name, metrics, final_count);

   return 0;
}