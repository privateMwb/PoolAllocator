#include "Pool.h"

#include "test_helper.h"

void run_constructor_tests();
void run_memory_allocation_tests();
void run_object_lifecycle_tests();
void run_pool_management_tests();
void run_introspection_tests();
void run_edge_cases_tests();

int main() {
    std::cout << "\n";
    run_constructor_tests();
    run_memory_allocation_tests();
    run_object_lifecycle_tests();
    run_pool_management_tests();
    run_introspection_tests();
    run_edge_cases_tests();
    
    stats();
    std::cout << "\n";
    return 0;
}

