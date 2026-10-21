#include <type_traits>

/**
 * Naive implementation of std::unique_ptr for learning purposes and stupid
 * interviews.
 */

namespace NaiveUniquePointerShit {
/* Custom deleter functor for custom (duh) deleters */
template <typename T> struct custom_deleter {
  void (*pf)(T *);
  custom_deleter(void (*pf)(T *)) : pf{pf} {}
  void operator()(T *pT) const /* noexcept; pf can throw :( */ { pf(pT); }
};

/* Default deleter functor for non-array objects/scalars */
template <typename T> struct default_deleter {
  void operator()(T *pT) const noexcept { delete pT; }
};

/* Default deleter functor for array objects/scalars */
template <typename T> struct default_deleter<T[]> {
  void operator()(T *pT) const noexcept { delete[] pT; }
};

template <typename T> struct is_deleter_function_candidate : std::false_type {};
template <typename T>
struct is_deleter_function_candidate<void (*)(T *)> : std::true_type {};
template <typename T>
constexpr auto is_deleter_function_candidate_v =
    is_deleter_function_candidate<T>::value;

}; // namespace NaiveUniquePointerShit

int main() { return 0; }
