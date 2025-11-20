# Progettazione di un Nodo FastFlow per l'Integrazione di Acceleratori Hardware

Questo progetto di tesi esplora l'integrazione efficiente di acceleratori hardware eterogenei (GPU e FPGA) all'interno del framework di streaming parallelo **FastFlow**.

Il core del progetto è un nuovo nodo FastFlow (**ff_node_acc_t**) progettato per gestire l'offloading asincrono di task computazionali su dispositivi eterogenei, nascondendo la 
latenza dei task dietro una pipeline di lavoro.

---

## Architettura

Il progetto adotta un'architettura software modulare basata sui design pattern **Strategy**, **Factory** e **Adapter**, per garantire flessibilità e manutenibilità.

### Strategie di Esecuzione (IDeviceRunner)

Un'interfaccia comune permette di eseguire lo stesso carico di lavoro su backend diversi senza modificare il codice client.

- **CPU**: Strategie per l'esecuzione parallela su CPU multicore (`Cpu_OMP_Runner`, `Cpu_FF_Runner`).
- **Acceleratori**: Un adattatore (`AcceleratorPipelineRunner`) incapsula una pipeline FastFlow specializzata per l'offloading.

### Pipeline di Offloading

Il nodo `ff_node_acc_t` implementa internamente una pipeline *Producer–Consumer* asincrona per massimizzare il throughput e sovrapporre la comunicazione *Host-to-Device* con il calcolo.

### Astrazione Hardware (IAccelerator)

Un'interfaccia unificata astrae le differenze tra le API di basso livello:

- OpenCL (FPGA / GPU NVIDIA / AMD)  
- Metal (Apple Silicon)

---

## Requisiti


### Dipendenze Software

- **CMake** (≥ 3.18)  
- Compilatore C++ con supporto **C++17**  
- **FastFlow**: scaricato automaticamente da CMake  
- **OpenCL**: per FPGA e GPU Apple M2 Pro
- **OpenMP**: richiesto per la strategia `cpu_omp` su Linux Xeon  
- **Metal**: richiesto per la strategia `gpu_metal` su MacOS  

---

## Setup Sperimentale

Le misurazioni sono state eseguite su due host di calcolo distinti:

1. **MacBook M2 Pro (ambiente di sviluppo)**  
   - Architettura: ARM64  
   - CPU: Apple M2 Pro, 10 core  
   - GPU: integrata Apple M2 Pro, 16 unità di calcolo  
   - Sistema operativo: macOS Sonoma 15.6.1  

2. **Host Linux**  
   - CPU: Intel Xeon E5-2650 v3 @ 2.30 GHz, 20 core fisici, 40 thread logici  
   - Sistema operativo: Ubuntu 22.04.5 LTS  
   - Acceleratore: scheda Xilinx Alveo U50 (xilinx_u50_gen3x16_xdma_base_5), connessa tramite bus PCIe

## Stack Software e Versioni

### Compilatore
- **MacOS:** Clang versione 20.1.7 [20] (Target: x86_64-apple-darwin24.6.0)  
- **Linux:** GCC 15.1.0 [21]

### Librerie e Toolchain
- **FastFlow:** versione 3.0.0 [10]  
  Utilizzata per implementare la pipeline sui nodi e per il confronto prestazionale su CPU Apple M2 Pro e CPU Intel.
- **OpenMP:** versione 4.5 [15]  
  Utilizzata per il confronto prestazionale su CPU Linux.
- **Toolchain Vitis:** versione v2023.1 [8]  
  Utilizzata per la Sintesi ad Alto Livello su FPGA.
- **API OpenCL:** versione 1.2 [9]  
  Utilizzata per l'interfacciamento con GPU Apple M2 Pro e FPGA.
- **Driver XRT:** versione 2.16.204 [19]  
  Driver di runtime Xilinx.
---

## Compilazione

Dalla directory principale del progetto:

```bash
rm -rf build; cmake -B build && cmake --build build
```

## Esecuzione
L'eseguibile ```tesi-exec``` accetta i seguenti parametri posizionali:

```bash
./build/tesi-exec <N> <NUM_TASKS> <DEVICE_TYPE> <KERNEL_ARG>
```


Parametri
- N: dimensione del problema (numero elementi)
- NUM_TASKS: numero di task da eseguire
- DEVICE_TYPE (backend supportati):
   - cpu_ff
   - cpu_omp
   - gpu_opencl
   - gpu_metal
   - fpga
- KERNEL_ARG:
   - CPU → nome kernel (vecAdd, polynomial_op, heavy_compute_kernel)
   - GPU/FPGA → percorso file .cl, .metal, .xclbin

<br>
Esempi
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
## Benchmark Automatizzati
Lo script incluso automatizza una suite di benchmark sui kernel disponibli e genera un CSV finale in /measurements:


```bash
chmod +x run_benchmarks.sh
./run_benchmarks.sh
```
