#pragma once

/**
 * @brief Definisce i nomi stringa costanti per i tipi di device.
 */
namespace device {

inline constexpr const char *GPU_CL = "gpu_opencl";
inline constexpr const char *GPU_MTL = "gpu_metal";
inline constexpr const char *FPGA = "fpga";
inline constexpr const char *CPU_FF = "cpu_ff";
inline constexpr const char *CPU_OMP = "cpu_omp";

} // namespace device