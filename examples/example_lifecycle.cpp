// Example Lifecycle
//
// Covers:
// - Typed object creation.
// - Object destruction.
// - Constructor argument forwarding.
// - Pool cleanup using reset.

#include "example_helper.h"

using namespace AllocatorPro;

struct Particle {
    float x, y, z;
    float velocity;

    Particle(float x, float y, float z, float velocity)
        : x(x), y(y), z(z), velocity(velocity)
    {
        std::cout << "Particle created  ("
                  << x << ", " << y << ", " << z
                  << ") v=" << velocity << "\n";
    }

    ~Particle() {
        std::cout << "Particle destroyed ("
                  << x << ", " << y << ", " << z << ")\n";
    }
};

int main() {
    mainTitle("\nLifecycle Examples");
    borderLine();

    // Object Creation
    setTitle("Object Creation");

    Pool pool{sizeof(Particle), 8};

    Particle* p1 = pool.create<Particle>(0.0f, 1.0f, 2.0f, 0.5f);
    Particle* p2 = pool.create<Particle>(3.0f, 4.0f, 5.0f, 1.0f);
    Particle* p3 = pool.create<Particle>(6.0f, 7.0f, 8.0f, 1.5f);

    std::cout << "\nUsed blocks : " << pool.usedBlocks() << "\n";
    std::cout << "Free blocks : " << pool.freeBlocks()  << "\n\n";

    // Object Destruction
    setTitle("Object Destruction");

    pool.destroy(p1);
    pool.destroy(p3);

    std::cout << "\nUsed blocks after destroy : " << pool.usedBlocks() << "\n";
    std::cout << "Free blocks after destroy : " << pool.freeBlocks()  << "\n\n";

    // Reuse
    setTitle("Reuse");

    Particle* p4 = pool.create<Particle>(9.0f, 10.0f, 11.0f, 2.0f);

    std::cout << "\nUsed blocks after reuse : " << pool.usedBlocks() << "\n";
    std::cout << "Free blocks after reuse : " << pool.freeBlocks()  << "\n\n";

    // Stats
    setTitle("Stats");

    Pool<true> tracked{sizeof(Particle), 8};

    Particle* s1 = tracked.create<Particle>(0.0f, 0.0f, 0.0f, 1.0f);
    Particle* s2 = tracked.create<Particle>(1.0f, 1.0f, 1.0f, 2.0f);
    tracked.destroy(s1);

    const auto& stats = tracked.getStats();

    std::cout << "\nAllocations   : " << stats.allocations_    << "\n";
    std::cout << "Deallocations : " << stats.deallocations_   << "\n";
    std::cout << "Peak used     : " << stats.peakUsed_        << "\n";
    std::cout << "Total alloc'd : " << stats.totalAllocated_  << "\n\n";

    // Bulk Cleanup
    setTitle("Bulk Cleanup");

    std::cout << "Used before reset : " << pool.usedBlocks() << "\n";
    pool.reset();
    std::cout << "Used after reset  : " << pool.usedBlocks() << "\n";
    std::cout << "Note: reset does not call destructors\n";

    (void)p2;
    (void)p4;
    (void)s2;

    borderLine();
    std::cout << "\n";
    return 0;
}