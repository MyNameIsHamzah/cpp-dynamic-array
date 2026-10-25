#!/bin/bash
# ==============================================================================
# Slurm Job Script: Cirrus HPC Benchmark Runner for dynamicArray
#
# Pinned execution on an isolated physical core to eliminate scheduler jitter,
# OS context switches, and CPU frequency scaling noise for HFT profiling.
# ==============================================================================
#SBATCH --job-name=dynamic_array_bench
#SBATCH --time=00:20:00
#SBATCH --nodes=1
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --output=benchmark_job_%j.out
#SBATCH --error=benchmark_job_%j.err

echo "=== Running on Cirrus HPC Node: $(hostname) ==="
echo "Start time: $(date)"

# Load required modern toolchain modules on Cirrus
module purge
module load gcc/12.2.0 cmake/3.25.2 python/3.10.8 2>/dev/null || module load gcc cmake python 2>/dev/null

# Clean build directory for Release performance build
mkdir -p build_bench
cd build_bench

echo "=== Configuring Release Build (No Sanitizers, -O3 optimizations) ==="
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ASAN=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_BENCHMARKS=ON

echo "=== Compiling Benchmark Suite ==="
cmake --build . --target benchmark_suite -j 4

echo "=== Executing Benchmarks (Pinned to core 0) ==="
# Pin task to CPU 0 to eliminate thread migration jitter
taskset -c 0 ./benchmark_suite \
    --benchmark_out=../benchmark_results_cirrus.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=3 \
    --benchmark_report_aggregates_only=true

cd ..

echo "=== Generating Visual Charts ==="
python3 scripts/plot_benchmarks.py benchmark_results_cirrus.json

echo "=== Benchmarks Complete ==="
echo "Results saved to benchmark_results_cirrus.json and charts to benchmarks/charts/"
echo "End time: $(date)"
