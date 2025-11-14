/*******************************************************************************
Description:
    This kernel implements a computationally-intensive (compute-bound)
    operation.

    This is a new "TASK-PARALLEL" architecture that replaces the previous
    "DATAFLOW" model.

    The model defined in krnl_heavy_compute.cpp used `dataflow` but had a single
    `compute_heavy` function that processed elements sequentially (in a loop).
    This created only ONE compute pipeline, which was a bottleneck.

    This new architecture creates `N_PIPELINES` (e.g. 8)
    parallel compute engines. It does this by:
    1.  Using a main loop (`main_loop_i`) that is pipelined (`II=1`).
    2.  Using an inner loop (`task_parallel_loop_k`) that is fully
        unrolled (`UNROLL`).

    This `UNROLL` pragma creates 8 physical, parallel copies of the
    `compute_scalar` function, allowing 8 elements to be processed
    simultaneously. This fully exploits the parallelism of the FPGA.

    The `load_input` and `store_result` functions are no longer needed
    because we are not using the `dataflow` model. The load/compute/store
    operations are now performed inside the unrolled loop.

*******************************************************************************/

#include <hls_math.h>   // For hls::sinf and hls::cosf
#include <hls_stream.h> // No longer used for streams, but good to include
#include <stdint.h>

#define DATA_SIZE 4096

// TRIPCOUNT identifier
const int c_size = DATA_SIZE;

// This controls how many parallel compute engines are created.
#define N_PIPELINES 8

/**
 * @brief The scalar compute engine.
 * This function processes *one single element* and is designed
 * to be instantiated `N_PIPELINES` times in parallel.
 */
static int32_t compute_scalar(int32_t a, int32_t b) {
   float result_f = 0.0f;
   float val_a = (float)a;
   float val_b = (float)b;

// Compute-intensive loop (5 iterations).
compute_loop:
   for (int j = 0; j < 5; ++j) {
      // We UNROLL this inner loop. Since it's small (5 iterations),
      // HLS will flatten it into 5 parallel compute stages.
#pragma HLS UNROLL
      result_f += hls::sinf(val_a + j) * hls::cosf(val_b - j);
   }

   return (int32_t)result_f;
}

extern "C" {

/**
 * @brief Top-level kernel (Task-Parallel Version).
 *
 * @param in1  (input)  --> Input vector 'a'
 * @param in2  (input)  --> Input vector 'b'
 * @param out  (output) --> Output vector 'c'
 * @param size (input)  --> Number of elements in vectors
 */
void krnl_heavy_compute_parallel(int32_t *in1, int32_t *in2, int32_t *out, int size) {
// Define memory interfaces
#pragma HLS INTERFACE m_axi port = in1 bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 bundle = gmem1
#pragma HLS INTERFACE m_axi port = out bundle = gmem0

   // This is the main loop.
   // It iterates through the data in "chunks" of N_PIPELINES.
main_loop_i:
   for (int i = 0; i < size; i += N_PIPELINES) {
#pragma HLS LOOP_TRIPCOUNT min = c_size / N_PIPELINES max = c_size / N_PIPELINES

      // This pragma pipelines the *main loop*.
      // II=1 means the loop will start a new iteration
      // (a new chunk of 8 elements) every clock cycle.
      // This provides the high throughput.
#pragma HLS PIPELINE II = 1

   // This inner loop is unrolled to create the parallel pipelines.
   task_parallel_loop_k:
      for (int k = 0; k < N_PIPELINES; k++) {
         // HLS will create 8 physical, parallel copies of all logic inside this loop.
#pragma HLS UNROLL

         int index = i + k;

         // Check boundaries (in case size is not a multiple of 8)
         if (index < size) {
            // LOAD: 8 parallel reads from global memory
            int32_t a = in1[index];
            int32_t b = in2[index];

            // COMPUTE: 8 parallel calls to the compute engine
            int32_t result = compute_scalar(a, b);

            // STORE: 8 parallel writes to global memory
            out[index] = result;
         }
      }
   }
}
}