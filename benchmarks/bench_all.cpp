#include "Pool.h"

void run_constructor_benchmarks();
void run_allocation_benchmarks();
void run_bulk_allocation_benchmarks();
void run_object_lifecycle_benchmarks();
void run_reset_benchmarks();
void run_throughput_benchmarks();

int main() {
    run_constructor_benchmarks();
    run_allocation_benchmarks();
    run_bulk_allocation_benchmarks();
    run_object_lifecycle_benchmarks();
    run_reset_benchmarks();
    run_throughput_benchmarks();

    return 0;
}

