// clang-format off
/**
 * compile with:
 * g++ -g naive_shared_ptr.cpp -o naive_shared_ptr -O2 -fsanitize=address,undefined
 */
// clang-format on

#include <atomic>
#include <iostream>
#include <random>
#include <thread>
#include <utility>
namespace NaiveSharedPointerShit {

template <typename T> class naive_shared_ptr {
private:
  T *ptr = nullptr;
  // this can't be a static variable because we don't want this to be shared for
  // each instance of a particular template instantiation. Instead, we want the
  // counter to be shared on copy constructor/assignment.
  //
  // As to why this should be atomic: a shared_ptr is usually used within a
  // concurrency environment because we usually don't who last owning object
  // is going to be. Consequently, different threads may concurrently access
  // and write to this counter.
  std::atomic<unsigned long long> *ctr = nullptr;

public:
  naive_shared_ptr() = default;

  // ctors

  // We take ownership of the pointee so if this constructor fails we free it.
  naive_shared_ptr(T *ptr) : ptr{ptr} {
    try {
      ctr = new std::atomic<unsigned long long>{1ULL};
    } catch (...) {
      delete ptr;
      throw;
    }
  }
  // Copy constructor
  naive_shared_ptr(const naive_shared_ptr &other) noexcept
      : ptr{other.ptr}, ctr{other.ctr} {
    if (ctr)
      ++(*ctr);
  }
  // Move constructor
  naive_shared_ptr(naive_shared_ptr &&other) noexcept
      : ptr{std::exchange(other.ptr, nullptr)},
        ctr{std::exchange(other.ctr, nullptr)} {}

  // dtor
  ~naive_shared_ptr() {
    if (!ctr)
      return;
    if ((*ctr)-- == 1) {
      delete ptr;
      ptr = nullptr;
      delete ctr;
      ctr = nullptr;
    }
  }

  naive_shared_ptr &operator=(const naive_shared_ptr &other) noexcept {
    naive_shared_ptr{other}.swap(*this);
    return *this;
  }

  naive_shared_ptr &operator=(naive_shared_ptr &&other) noexcept {
    naive_shared_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  void swap(naive_shared_ptr &other) noexcept {
    std::swap(ptr, other.ptr);
    std::swap(ctr, other.ctr);
  }

  bool empty() const noexcept { return ptr == nullptr; }
  operator bool() const noexcept { return !empty(); }

  bool operator==(const naive_shared_ptr &other) const noexcept {
    return ptr == other.ptr;
  }

  T *get() noexcept { return ptr; }
  const T *get() const noexcept { return ptr; }
  T &operator*() noexcept { return *ptr; }
  const T &operator*() const noexcept { return *ptr; }
  T *operator->() noexcept { return ptr; }
  const T *operator->() const noexcept { return ptr; }
};
}; // namespace NaiveSharedPointerShit

using namespace std::literals;

struct X {
  int n;
  X(int n) : n{n} {}
  ~X() { std::cout << "X::~X()" << std::endl; }
};

int main() {
  using namespace NaiveSharedPointerShit;
  std::mt19937 prng{std::random_device{}()};
  std::uniform_int_distribution<int> die{200, 300};
  naive_shared_ptr<X> p{new X{3}};

  using std::chrono::milliseconds;

  std::thread th0{[p, dt = die(prng)] {
    std::this_thread::sleep_for(milliseconds(dt));
    std::cout << "end of th0, p->n: " << p->n << std::endl;
  }};

  std::thread th1{[p, dt = die(prng)] {
    std::this_thread::sleep_for(milliseconds{dt});
    std::cout << "end of th1, p->n: " << p->n << std::endl;
  }};

  th0.detach();
  th1.detach();
  // sleep until all threads are finished (> 300ms)
  std::this_thread::sleep_for(350ms);
  std::cout << "end main()" << std::endl;

  return 0;
}
