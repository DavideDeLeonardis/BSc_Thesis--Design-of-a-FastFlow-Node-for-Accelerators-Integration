/**
 * Implementazione della Factory per la creazione di uno specifico DeviceRunner fra
 * CPU FastFlow, CPU OpenMP, GPU OpenCL, GPU Metal, FPGA.
 */

#include "DeviceRunner_Factory.hpp"

#include "../strategy_accelerator/AcceleratorPipelineRunner.hpp"
#include "../strategy_cpu/Cpu_FF_Runner.hpp"

#ifdef __APPLE__
#include "../strategy_accelerator/accelerator/Gpu_Metal_Accelerator.hpp"
#include "../strategy_accelerator/accelerator/Gpu_OpenCL_Accelerator.hpp"
#else
#include "../strategy_accelerator/accelerator/Fpga_Accelerator.hpp"
#include "../strategy_cpu/Cpu_OMP_Runner.hpp"
#endif

std::unique_ptr<IDeviceRunner> create_runner_for_device(const std::string &device_type,
                                                        const std::string &kernel_path,
                                                        const std::string &kernel_name) {
   if (device_type == "cpu_ff") {
      return std::make_unique<Cpu_FF_Runner>(kernel_name);
   }

#ifdef __APPLE__

   else if (device_type == "gpu_opencl") {
      auto accelerator = std::make_unique<Gpu_OpenCL_Accelerator>(kernel_path, kernel_name);
      return std::make_unique<AcceleratorPipelineRunner>(std::move(accelerator));
   }

   else if (device_type == "gpu_metal") {
      auto accelerator = std::make_unique<Gpu_Metal_Accelerator>(kernel_path, kernel_name);
      return std::make_unique<AcceleratorPipelineRunner>(std::move(accelerator));
   }

#else

   else if (device_type == "cpu_omp") {
      return std::make_unique<Cpu_OMP_Runner>(kernel_name);
   }

   else if (device_type == "fpga") {
      auto accelerator = std::make_unique<Fpga_Accelerator>(kernel_path, kernel_name);
      return std::make_unique<AcceleratorPipelineRunner>(std::move(accelerator));
   }
#endif

   return nullptr;
}