// Embedding Pool inside a domain-specific class.
//
// Demonstrates:
// - Wrapping Pool as a private implementation detail
// - Exposing a narrow, purpose-built API instead of raw allocate/deallocate
// - Callers of the wrapper never seeing Pool at all

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Particle {
    float x, y, vx, vy;
};

// A tiny particle system that hands out and reclaims particle slots. Pool
// is purely an implementation detail — the public API talks only about
// particles.
class ParticleSystem {
  public:
    explicit ParticleSystem(std::size_t maxParticles) : pool_(sizeof(Particle), maxParticles) {}

    Particle* spawn(float x, float y, float vx, float vy) {
        return pool_.create<Particle>(x, y, vx, vy);
    }

    void kill(Particle* p) {
        pool_.destroy(p);
    }

    [[nodiscard]] std::size_t alive() const {
        return pool_.usedBlocks();
    }
    [[nodiscard]] std::size_t capacity() const {
        return pool_.totalBlocks();
    }

  private:
    Pool<> pool_;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    ParticleSystem particles(100);
    std::cout << "capacity: " << particles.capacity() << "\n";
    std::cout << "alive   : " << particles.alive() << "\n\n";

    setTitle("Spawning Particles");

    Particle* p1 = particles.spawn(0.0f, 0.0f, 1.0f, 0.0f);
    Particle* p2 = particles.spawn(1.0f, 1.0f, -1.0f, 0.5f);

    std::cout << "p1: (" << p1->x << ", " << p1->y << ")\n";
    std::cout << "p2: (" << p2->x << ", " << p2->y << ")\n";
    std::cout << "alive: " << particles.alive() << "\n\n";

    setTitle("Killing a Particle");

    particles.kill(p1);
    std::cout << "alive: " << particles.alive() << "\n";
}

REGISTER_EXAMPLE_SUITE();
