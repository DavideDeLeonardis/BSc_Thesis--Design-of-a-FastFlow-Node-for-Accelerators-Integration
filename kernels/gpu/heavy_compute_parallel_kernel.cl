/*******************************************************************************
Description:
    OpenCL version of the heavy_compute kernel.

    This kernel is written in the "Data-Parallel" model.
    The host application (not shown) is responsible for
    launching `size` work-items in parallel.

    *** FIX: Removed "#pragma unroll 5" because the ***
    *** cl2Metal transpiler on macOS cannot handle it. ***
*******************************************************************************/

/**
 * @brief The scalar compute engine.
 * This function processes *one single element*.
 */
static int compute_scalar(int a, int b) {
    // Use 'float' for calculations.
    float result_f = 0.0f;
    float val_a = (float)a;
    float val_b = (float)b;

// Compute-intensive loop (5 iterations)
compute_loop:
    for (int j = 0; j < 5; ++j) {
        // #pragma unroll 5 <-- REMOVED. This was causing cl2Metal to fail.
        result_f += sin(val_a + j) * cos(val_b - j);
    }

    return (int)result_f;
}

/**
 * @brief Top-level OpenCL kernel (Data-Parallel Version).
 * Each instance of this kernel is one "work-item".
 *
 * The kernel name is set to "heavy_compute_parallel_kernel"
 * to match the host application's request.
 */
__kernel void heavy_compute_parallel_kernel(
    const __global int* in1,
    const __global int* in2,
    __global int* out,
    const int size)
{
    // Get the unique ID for this work-item.
    int index = get_global_id(0);

    // Boundary check
    if (index < size) {
        // LOAD: Read one element
        int a = in1[index];
        int b = in2[index];

        // COMPUTE: Call the compute engine for this one element
        int result = compute_scalar(a, b);

        // STORE: Write one element
        out[index] = result;
    }
}