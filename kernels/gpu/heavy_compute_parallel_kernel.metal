/*******************************************************************************
Description:
    Metal Shading Language (MSL) version of the heavy_compute kernel.

    *** FIX (Attempt 8) ***
    The previous errors (`invalid type 'int'`, `invalid type 'uint'`)
    were misleading. They failed because the host (CPU) is passing
    the 'size' argument using `setBytes` at buffer index 3.

    The kernel was rejecting this.
    1. `int size [[buffer(3)]]` failed because `int` is not a buffer pointer.
    2. `int size` failed because the host provided data at `buffer(3)`.

    FIX: The correct syntax for data passed via `setBytes` is to
    use the `constant` address space and a *reference* (`&`).
    The signature is changed to:
    `constant int& size [[buffer(3)]]`
*******************************************************************************/

#include <metal_stdlib>
using namespace metal;

/**
 * @brief The scalar compute engine.
 */
int compute_scalar(int a, int b) {
    // Use 'float' for calculations.
    float result_f = 0.0f;
    float val_a = (float)a;
    float val_b = (float)b;

    // `compute_loop:` label was removed (not valid in MSL)
    for (int j = 0; j < 5; ++j) {
        // Explicitly cast 'j' to float(j)
        result_f += sin(val_a + float(j)) * cos(val_b - float(j));
    }

    return (int)result_f;
}

/**
 * @brief Top-level Metal kernel (Data-Parallel Version).
 * Kernel name is "heavy_compute_parallel_kernel"
 */
kernel void heavy_compute_parallel_kernel(
    const device int* in1   [[buffer(0)]],
    const device int* in2   [[buffer(1)]],
    device int* out         [[buffer(2)]],
    // *** FIX: This is the correct syntax for 'setBytes' ***
    constant int& size      [[buffer(3)]],
    uint index              [[thread_position_in_grid]])
{
    // Boundary check
    // Cast 'size' (int) to 'uint' for a safe comparison
    if (index < (uint)size) {
        // LOAD: Read one element
        int a = in1[index];
        int b = in2[index];

        // COMPUTE: Call the compute engine for this one element
        int result = compute_scalar(a, b);

        // STORE: Write one element
        out[index] = result;
    }
}