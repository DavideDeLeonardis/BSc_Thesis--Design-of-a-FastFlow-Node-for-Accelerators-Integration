# Design of a FastFlow Node for Hardware Accelerators Integration

This thesis project explores the efficient integration of heterogeneous hardware accelerators (GPUs and FPGAs) within the **FastFlow** parallel streaming framework.

The core of the project is a new FastFlow node (**ff_node_acc_t**) designed to handle the asynchronous offloading of computational tasks to heterogeneous devices, hiding task latency behind a work pipeline.

---

## Architecture

The project adopts a modular software architecture based on the **Strategy**, **Factory**, and **Adapter** design patterns to ensure flexibility and maintainability.

### Execution Strategies (IDeviceRunner)

A common interface allows executing the same workload on different backends without modifying client code.

- **CPU**: Strategies for parallel execution on multicore CPUs (`Cpu_OMP_Runner`, `Cpu_FF_Runner`).
- **Accelerators**: An adapter (`AcceleratorPipelineRunner`) encapsulates a specialized FastFlow pipeline for offloading.

### Offloading Pipeline

The `ff_node_acc_t` node internally implements an asynchronous *Producer–Consumer* pipeline to maximize throughput and overlap *Host-to-Device* communication with computation.

### Hardware Abstraction (IAccelerator)

A unified interface abstracts the differences between low-level APIs:

- OpenCL (FPGA / GPU MacBook M2 Pro)
- Metal (Apple Silicon)

---

## Requirements


### Software Dependencies

- **CMake** (≥ 3.18)
- C++ Compiler with **C++17** support
- **FastFlow**: automatically downloaded by CMake
- **OpenCL**: for FPGA and Apple M2 Pro GPU
- **OpenMP**: required for the `cpu_omp` strategy on Linux Xeon
- **Metal**: required for the `gpu_metal` strategy on MacOS

---

## Experimental Setup

Measurements were performed on two distinct compute hosts:

1. **MacBook M2 Pro (development environment)**
   - Architecture: ARM64
   - CPU: Apple M2 Pro, 10 cores
   - GPU: integrated Apple M2 Pro, 16 compute units
   - OS: macOS Sonoma 15.6.1

2. **Linux Host**
   - CPU: Intel Xeon E5-2650 v3 @ 2.30 GHz, 20 physical cores, 40 logical threads
   - OS: Ubuntu 22.04.5 LTS
   - Accelerator: Xilinx Alveo U50 card (xilinx_u50_gen3x16_xdma_base_5), connected via PCIe bus

## Software Stack and Versions

### Compiler
- **MacOS:** Clang version 20.1.7 (Target: x86_64-apple-darwin24.6.0)
- **Linux:** GCC 15.1.0

### Libraries and Toolchain
- **FastFlow:** version 3.0.0 
  Used to implement the pipeline on nodes and for performance comparison on Apple M2 Pro CPU and Intel CPU.
- **OpenMP:** version 4.5
  Used for performance comparison on Linux CPU.
- **Vitis Toolchain:** version v2023.1
  Used for High-Level Synthesis on FPGA.
- **OpenCL API:** version 1.2
  Used for interfacing with Apple M2 Pro GPU and FPGA.
- **XRT Drivers:** version 2.16.204
  Xilinx runtime drivers.
---

## Compilation

From the main project directory:

```bash
rm -rf build; cmake -B build && cmake --build build
```

## Execution
The executable `tesi-exec` accepts the following positional parameters:

```bash
./build/tesi-exec <N> <NUM_TASKS> <DEVICE_TYPE> <KERNEL_ARG>
```


Parameters
- N: problem size (number of elements)
- NUM_TASKS: number of tasks to execute
- DEVICE_TYPE (supported backends):
   - cpu_ff
   - cpu_omp
   - gpu_opencl
   - gpu_metal
   - fpga
- KERNEL_ARG:
   - CPU → kernel name (vecAdd, polynomial_op, heavy_compute_kernel)
   - GPU/FPGA → path to .cl, .metal, .xclbin file

<br>
Examples
CPU (FastFlow):

```bash
./build/tesi-exec 1000000 100 cpu_ff polynomial_op
```

GPU (Metal - macOS):

```bash
./build/tesi-exec 1000000 100 gpu_metal kernels/gpu/heavy_compute_kernel.metal
```
FPGA (OpenCL - Linux):

```bash
./build/tesi-exec 1000000 100 fpga kernels/fpga/krnl_vadd.xclbin
```
## Automated Benchmarks
The included script automates a benchmark suite on available kernels and generates a final CSV in /measurements:


```bash
chmod +x run_benchmarks.sh
./run_benchmarks.sh
```