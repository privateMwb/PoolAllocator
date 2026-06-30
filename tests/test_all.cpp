#include "test_helper.h"

void run_constructor_tests();
void run_move_tests();
void run_memory_tests();
void run_lifecycle_tests();
void run_state_tests();
void run_stats_tests();
void run_introspection_tests();
void run_edge_tests();

int main() {
    std::cout << "\n";

    run_constructor_tests();
    run_move_tests();
    run_memory_tests();
    run_lifecycle_tests();
    run_state_tests();
    run_stats_tests();
    run_introspection_tests();
    run_edge_tests();

    stats();
    std::cout << "\n";

    return 0;
}

