// Pool statistics example.
//
// Demonstrates:
// - Allocation statistics tracking
// - Peak usage tracking
// - Statistics after deallocation
// - Statistics after reset

#include "example_helper.h"

using namespace AllocatorPro;

int main() {
    mainTitle("\nStats Examples");
    borderLine();

    Pool<true> pool{64, 8};

    // Initial statistics.
    setTitle("Initial Statistics");

    std::cout << "Total allocated : " << pool.getStats().totalAllocated_ << "\n";
    std::cout << "Peak used       : " << pool.getStats().peakUsed_       << "\n";
    std::cout << "Allocations     : " << pool.getStats().allocations_    << "\n";
    std::cout << "Deallocations   : " << pool.getStats().deallocations_  << "\n\n";

    // Allocation statistics tracking.
    setTitle("Tracking Allocations");

    void* p1 = pool.allocate();
    std::cout << "After 1st allocation:\n";
    std::cout << "  Total allocated : " << pool.getStats().totalAllocated_ << "\n";
    std::cout << "  Peak used       : " << pool.getStats().peakUsed_       << "\n";
    std::cout << "  Allocations     : " << pool.getStats().allocations_    << "\n\n";

    void* p2 = pool.allocate();
    std::cout << "After 2nd allocation:\n";
    std::cout << "  Total allocated : " << pool.getStats().totalAllocated_ << "\n";
    std::cout << "  Peak used       : " << pool.getStats().peakUsed_       << "\n";
    std::cout << "  Allocations     : " << pool.getStats().allocations_    << "\n\n";

    // Peak usage tracking.
    setTitle("Peak Used Tracking");

    void* p3 = pool.allocate();
    std::cout << "Peak after 3rd allocation : " << pool.getStats().peakUsed_ << "\n";

    pool.deallocate(p3);
    std::cout << "Peak after deallocate     : " << pool.getStats().peakUsed_ << "\n";
    std::cout << "Used blocks after free    : " << pool.usedBlocks()        << "\n\n";

    // Deallocation statistics tracking.
    setTitle("Tracking Deallocations");

    pool.deallocate(p1);
    pool.deallocate(p2);

    std::cout << "Deallocations : " << pool.getStats().deallocations_ << "\n";
    std::cout << "Peak used     : " << pool.getStats().peakUsed_      << "\n\n";

    // Statistics after reset.
    setTitle("Stats After Reset");

    (void)pool.allocate();
    (void)pool.allocate();

    std::cout << "Total allocated before reset : " << pool.getStats().totalAllocated_ << "\n";
    std::cout << "Allocations before reset     : " << pool.getStats().allocations_    << "\n";
    std::cout << "Peak used before reset       : " << pool.getStats().peakUsed_       << "\n";

    pool.reset();

    std::cout << "Total allocated after reset  : " << pool.getStats().totalAllocated_ << "\n";
    std::cout << "Allocations after reset      : " << pool.getStats().allocations_    << "\n";
    std::cout << "Peak used after reset        : " << pool.getStats().peakUsed_       << "\n";
    std::cout << "Note: peakUsed_ is not preserved across reset for Pool\n";

    borderLine();
    std::cout << "\n";
    return 0;
}
