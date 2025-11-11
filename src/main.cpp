/**
 * @file main.cpp
 * @brief Questo file orchestra l'intera applicazione, non conosce i dettagli implementativi
 * dei vari tipi di calcolo.
 *
 * Utilizza due pattern di design principali:
 * 1. Factory Pattern: Per delegare la creazione dell'oggetto corretto IDeviceRunner tramite la
 * factory DeviceRunner_Factory in base al tipo di dispositivo scelto (CPU, GPU, FPGA).
 * 2. Strategy Pattern: Per eseguire il calcolo senza sapere quale implementazione concreta
 * (CPU, Accelerator) sia stata scelta.
 */

#include "common/ComputeResult.hpp"
#include "common/IDeviceRunner.hpp"
#include "common/device_types.h"
#include "factory/DeviceRunner_Factory.hpp"

#include "helpers/Helpers.hpp"

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char *argv[]) {
   // Parametri della command line di default.
   size_t N = 1000000, NUM_TASKS = 20;
   std::string device_type = device::CPU_FF,  
               kernel_path = "", kernel_name = "";

   // Parsing degli argomenti della command line (setta anche il kernel di default) e stampa
   // della configurazione scelta.
   parse_args(argc, argv, N, NUM_TASKS, device_type, kernel_path, kernel_name);
   print_configuration(N, NUM_TASKS, device_type, kernel_path, kernel_name);

   ComputeResult results;

   {
      // Delega alla Factory la creazione della strategia di esecuzione corretta
      // (Cpu_OMP_Runner, AcceleratorPipelineRunner, ecc.) in base al device_type.
      std::unique_ptr<IDeviceRunner> strategy =
         create_runner_for_device(device_type, kernel_path, kernel_name);

      if (!strategy) {
         std::cerr << "[ERROR] Invalid device type '" << device_type << "' for this OS.\n";
         print_usage(argv[0]);
         return -1;
      }

      // Esecuzione della computazione tramite la Strategy scelta (CPU/GPU/FPGA) che esegue la
      // parallelizzazione dei task su CPU multicore o la pipeline con offloading su GPU/FPGA.
      results = strategy->execute(N, NUM_TASKS);
   }

   // Calcolo e stampa delle metriche di performance finali.
   PerformanceData metrics = calculate_metrics(results);
   print_metrics(N, NUM_TASKS, device_type, kernel_name, metrics, results.tasks_completed);

   return 0;
}