// Constructing non-trivial types through create<T>().
//
// Demonstrates:
// - Forwarding constructor arguments through create<T>() for a type with
//   several members and a non-trivial constructor
// - create<T>() working the same way regardless of how complex T's
//   constructor is

#include <support/framework.h>

#include <string>

using namespace PoolPro;

namespace {

struct Employee {
    std::string name;
    int id;
    double salary;

    Employee(std::string name_, int id_, double salary_)
        : name(std::move(name_)), id(id_), salary(salary_) {}
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(sizeof(Employee), 8);
    std::cout << "blockStride: " << pool.blockStride() << "\n\n";

    // create<T>() forwards its arguments straight to Employee's
    // constructor — the pool doesn't need to know anything about Employee
    // beyond its size and alignment.
    setTitle("Forwarding Constructor Arguments");

    Employee* e = pool.create<Employee>("Ada Lovelace", 1, 95000.0);

    std::cout << "name  : " << e->name << "\n";
    std::cout << "id    : " << e->id << "\n";
    std::cout << "salary: " << e->salary << "\n\n";

    // destroy<T>() runs Employee's destructor — releasing the std::string's
    // own heap buffer — before returning the block to the pool.
    setTitle("Destruction");

    pool.destroy(e);
    std::cout << "usedBlocks after destroy: " << pool.usedBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
