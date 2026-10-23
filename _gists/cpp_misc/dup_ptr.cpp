// clang-format off
/**
 * compile with:
 * g++ -g naive_shared_ptr.cpp -o naive_shared_ptr -O2 -fsanitize=address,undefined
 */
// clang-format on

#include <type_traits>
#include <concepts>

namespace DuplicatePointerShit {

struct Copier {
  template <typename T> T *operator()(const T *p) const { return new T{*p}; }
};

struct Cloner {
  template <typename T> T *operator()(const T *p) const { return p->clone(); }
};

#if 0 /* intrusive solution */
struct cloneable {
  virtual cloneable *clone() const = 0;
  virtual ~cloneable() = default;
};

template <typename T, typename Dup = std::conditional_t<
                          std::is_base_of_v<cloneable, T>, Cloner, Copier>>
class dup_ptr {
private:
  T *ptr = nullptr;

public:
  dup_ptr(const dup_ptr &other) noexcept
      : ptr{other.empty() ? nullptr : Dup{}(other.ptr)} {}

  bool empty() const noexcept { return ptr == nullptr; }
};
#else /* non-intrusive; relies on C++20 concepts */
template <typename T>
concept cloneable = requires(const T *p) {
  { p->clone() } -> std::convertible_to<T *>;
};

template <typename T,
          typename Dup = std::conditional_t<cloneable<T>, Cloner, Copier>>
class dup_ptr {
private:
  T *ptr = nullptr;

public:
  dup_ptr(const dup_ptr &other) noexcept
      : ptr{other.empty() ? nullptr : Dup{}(other.ptr)} {}

  bool empty() const noexcept { return ptr == nullptr; }
};
#endif
} // namespace DuplicatePointerShit

int main() { return 0; }
