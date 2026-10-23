#include <iostream>
namespace NonNullPtrShit {
class invalid_ptr {};

template <typename T> class non_null_ptr {
private:
  T *ptr;

public:
  non_null_ptr() = delete;
  non_null_ptr(T *p) : ptr{p} {
    if (!ptr) {
      throw invalid_ptr{};
    }
  }

  T *get() noexcept { return ptr; }
  const T *get() const noexcept { return ptr; }

  T &operator*() noexcept { return *ptr; }
  const T &operator*() const noexcept { return *ptr; }
  T *operator->() noexcept { return ptr; }
  const T *operator->() const noexcept { return ptr; }
};
}; // namespace NonNullPtrShit

using namespace NonNullPtrShit;

struct X {
  int n;
};

int extract_value(const non_null_ptr<X> &p) {
  // We don't need to validate anything here; p is guaranteed to be non null
  return p->n;
}

int main() {
  X x{3};
  try {
    std::cout << extract_value(&x) << '\n'
              << extract_value(nullptr) << std::endl;
  } catch (const invalid_ptr &) {
    std::cerr << "oopsie!" << std::endl;
  }
  return 0;
}
