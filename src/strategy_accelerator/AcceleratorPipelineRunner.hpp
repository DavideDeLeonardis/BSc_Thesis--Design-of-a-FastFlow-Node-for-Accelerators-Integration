#pragma once

#include "../common/IDeviceRunner.hpp"
#include "./accelerator/IAccelerator.hpp"
#include <memory>

/**
 * @brief Strategia concreta che implementa IDeviceRunner (è anche un Adapter).
 *
 * Questa classe "adatta" la logica della pipeline FastFlow + Acceleratore per farla apparire
 * come una semplice strategia eseguibile dal main.
 *
 */
class AcceleratorPipelineRunner : public IDeviceRunner {
 public:
   /**
    * @brief Costruttore che prende possesso dell'acceleratore hardware da usare.
    */
   explicit AcceleratorPipelineRunner(std::unique_ptr<IAccelerator> accelerator);

   virtual ~AcceleratorPipelineRunner() = default;

   /**
    * @brief Orchestra l'intera pipeline FastFlow per l'offloading su un acceleratore. Crea i
    * due nodi della pipeline FF (Emitter, ff_node_acc_t). Riceve l'acceleratore già
    * inizializzato. Avvia la pipeline. Misura e raccoglie i tempi di esecuzione e il numero di
    * task completati.
    */
   ComputeResult execute(size_t N, size_t NUM_TASKS) override;

 private:
   std::unique_ptr<IAccelerator> accelerator_;
};