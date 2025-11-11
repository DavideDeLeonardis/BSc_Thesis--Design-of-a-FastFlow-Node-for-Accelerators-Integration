#!/bin/bash

# Script per eseguire tutti i benchmark e raccogliere i risultati in un file CSV.

OUTPUT_DIR="measurement"
OUTPUT_FILE="$OUTPUT_DIR/Measurements.csv"
N_VALUES=(10000 1000000 7449999)
NUM_TASKS=100

# Controlla se il file eseguibile esiste. Se non esiste, avvia la build automatica.
EXECUTABLE="./build/tesi-exec"
if [ ! -f "$EXECUTABLE" ]; then
    echo "--- Eseguibile non trovato: $EXECUTABLE ---"
    echo "Avvio della build."
    
    rm -rf build
    cmake -B build && cmake --build build
    
    if [ $? -ne 0 ]; then
        echo "ERRORE: Compilazione fallita."
        exit 1
    fi
    
    if [ ! -f "$EXECUTABLE" ]; then
         echo "ERRORE: Compilazione riuscita ma l'eseguibile non è stato trovato in $EXECUTABLE."
         exit 1
    fi
    
    echo "Compilazione completata con successo."
    echo
    echo "--- Inizio dei Benchmark ---"
    echo
else
    echo "--- Inizio dei Benchmark ---"
    echo
fi

# Pulisce il file CSV precedente e scrive l'intestazione.
echo "OS,N,Tasks,Device,Kernel,Avg_Service_Time_ms,Avg_In_Node_Time_ms,Avg_Compute_Time_ms,Avg_Overhead_Time_ms,Throughput_tasks_s,Total_Time_s,Status" > $OUTPUT_FILE

# Funzione helper per eseguire un singolo test e fare il parsing dell'output.
run_test() {
    local N=$1
    local DEVICE=$2
    local KERNEL_ARG=$3
    local KERNEL_NAME=$4
    local OS_NAME=$5

    echo "Running: OS=$OS_NAME, N=$N, Device=$DEVICE, Kernel=$KERNEL_NAME"

    # Esegue il comando e cattura sia stdout che stderr.
    output=$( $EXECUTABLE $N $NUM_TASKS $DEVICE $KERNEL_ARG 2>&1 )
    
    # Controlla se l'esecuzione è fallita.
    if [ $? -ne 0 ]; then
        echo "Run FAILED for $DEVICE, $KERNEL_NAME, N=$N"
        echo "$OS_NAME,$N,$NUM_TASKS,$DEVICE,$KERNEL_NAME,,,,,,,FAILED" >> $OUTPUT_FILE
        echo "$output"
        return
    fi

    # --- Parsing delle metriche dall'output ---
    SERVICE_TIME=$(echo "$output" | grep "Avg Service Time" | awk -F: '{print $2}' | awk '{print $1}')
    IN_NODE_TIME=$(echo "$output" | grep "Avg In_Node Time" | awk -F: '{print $2}' | awk '{print $1}')
    COMPUTE_TIME=$(echo "$output" | grep "Avg Pure Compute Time" | awk -F: '{print $2}' | awk '{print $1}')
    OVERHEAD_TIME=$(echo "$output" | grep "Avg Overhead Time" | awk -F: '{print $2}' | awk '{print $1}')
    THROUGHPUT=$(echo "$output" | grep "Throughput" | awk -F: '{print $2}' | awk '{print $1}')
    TOTAL_TIME=$(echo "$output" | grep "Total Time Elapsed" | awk -F: '{print $2}' | awk '{print $1}')

    # Scrive la riga CSV.
    echo "$OS_NAME,$N,$NUM_TASKS,$DEVICE,$KERNEL_NAME,$SERVICE_TIME,$IN_NODE_TIME,$COMPUTE_TIME,$OVERHEAD_TIME,$THROUGHPUT,$TOTAL_TIME,Success" >> $OUTPUT_FILE
}

# Rileva il sistema operativo.
OS_NAME=$(uname -s)

if [ "$OS_NAME" == "Darwin" ]; then
    # --- Comandi MacOS ---
    OS_NAME="MacOS"
    DEVICES=("cpu_ff" "gpu_opencl" "gpu_metal")
    
    for N in "${N_VALUES[@]}"; do
        for DEVICE in "${DEVICES[@]}"; do
            if [ "$DEVICE" == "cpu_ff" ]; then
                run_test $N "cpu_ff" "vecAdd" "vecAdd" $OS_NAME
                run_test $N "cpu_ff" "polynomial_op" "polynomial_op" $OS_NAME
                run_test $N "cpu_ff" "heavy_compute_kernel" "heavy_compute_kernel" $OS_NAME
            
            elif [ "$DEVICE" == "gpu_opencl" ]; then
                run_test $N "gpu_opencl" "kernels/gpu/vecAdd.cl" "vecAdd" $OS_NAME
                run_test $N "gpu_opencl" "kernels/gpu/polynomial_op.cl" "polynomial_op" $OS_NAME
                run_test $N "gpu_opencl" "kernels/gpu/heavy_compute_kernel.cl" "heavy_compute_kernel" $OS_NAME
            
            elif [ "$DEVICE" == "gpu_metal" ]; then
                run_test $N "gpu_metal" "kernels/gpu/vecAdd.metal" "vecAdd" $OS_NAME
                run_test $N "gpu_metal" "kernels/gpu/polynomial_op.metal" "polynomial_op" $OS_NAME
                run_test $N "gpu_metal" "kernels/gpu/heavy_compute_kernel.metal" "heavy_compute_kernel" $OS_NAME
            fi
        done
    done

elif [ "$OS_NAME" == "Linux" ]; then
    # --- Comandi Linux VM ---
    OS_NAME="Linux"
    DEVICES=("cpu_ff" "cpu_omp" "fpga")

    for N in "${N_VALUES[@]}"; do
        for DEVICE in "${DEVICES[@]}"; do
            if [ "$DEVICE" == "cpu_ff" ]; then
                run_test $N "cpu_ff" "vecAdd" "vecAdd" $OS_NAME
                run_test $N "cpu_ff" "polynomial_op" "polynomial_op" $OS_NAME
                run_test $N "cpu_ff" "heavy_compute_kernel" "heavy_compute_kernel" $OS_NAME
            
            elif [ "$DEVICE" == "cpu_omp" ]; then
                run_test $N "cpu_omp" "vecAdd" "vecAdd" $OS_NAME
                run_test $N "cpu_omp" "polynomial_op" "polynomial_op" $OS_NAME
                run_test $N "cpu_omp" "heavy_compute_kernel" "heavy_compute_kernel" $OS_NAME
            
            elif [ "$DEVICE" == "fpga" ]; then
                run_test $N "fpga" "kernels/fpga/krnl_vadd.xclbin" "vecAdd" $OS_NAME
                run_test $N "fpga" "kernels/fpga/krnl_polynomial_op.xclbin" "polynomial_op" $OS_NAME
                run_test $N "fpga" "kernels/fpga/krnl_heavy_compute.xclbin" "heavy_compute_kernel" $OS_NAME
            fi
        done
    done

else
    echo "Sistema operativo non supportato: $OS_NAME"
    exit 1
fi

echo "----------------------------------------"
echo "Benchmark completati."
echo "Risultati salvati in: $OUTPUT_FILE"
echo "----------------------------------------"