/**
 * compile with:
 * g++ -g naive_unique_ptr.cpp -o naive_unique_ptr -O2
 * -fsanitize=address,undefined
 */

#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * Naive implementation of std::unique_ptr for learning purposes and stupid
 * interviews.
 */

namespace NaiveUniquePointerShit {

/* Custom deleter functor for custom (duh) deleters */
template <typename T> struct custom_deleter_wrapper {
  void (*pf)(T *);
  custom_deleter_wrapper(void (*pf)(T *)) : pf{pf} {}
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

/// Naive unique ptr scalar specialization. The idea here is to inherit from
/// base class D (i.e., deleter type) and treat it as a functor with the
/// function pointer type void (*)(T*). This will be called in the destructor.
///
/// If a custom deleter is not supplied then we default to the default deleter
/// for the type T which is `delete ptr`.
///
/// If a custom deleter is supplied, it is wrapped into a functor which in turn
/// is inherited from and called in the destructor.
///
template <typename T, typename D = default_deleter<T>>
class naive_unique_ptr : std::conditional_t<is_deleter_function_candidate_v<D>,
                                            custom_deleter_wrapper<T>, D> {
  using deleter_type = std::conditional_t<is_deleter_function_candidate_v<D>,
                                          custom_deleter_wrapper<T>, D>;
  T *ptr = nullptr;

public:
  naive_unique_ptr() = default;
  naive_unique_ptr(T *p) : ptr{p} {}
  naive_unique_ptr(T *p, void (*pf)(T *)) : deleter_type{pf}, ptr{p} {}

  naive_unique_ptr(const naive_unique_ptr &) = delete;
  naive_unique_ptr &operator=(const naive_unique_ptr &) = delete;

  void swap(naive_unique_ptr &other) noexcept { std::swap(ptr, other.ptr); }

  naive_unique_ptr(naive_unique_ptr &&other) noexcept {
    std::swap(ptr, other.ptr);
  }
  naive_unique_ptr &operator=(naive_unique_ptr &&other) noexcept {
    // just delegate this to our swap by swaping other with a tmp object
    naive_unique_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  ~naive_unique_ptr() { (*static_cast<deleter_type *>(this))(ptr); }

  inline bool empty() const noexcept { return ptr == nullptr; }
  operator bool() const noexcept { return !empty(); }
  bool operator==(const naive_unique_ptr &other) const noexcept {
    return ptr == other.ptr;
  }

  // inferred since C++20
  // bool operator!=(const naive_unique_ptr &other) const noexcept {
  //   return !(*this == other);
  // }

  T *get() noexcept { return ptr; }
  const T *get() const noexcept { return ptr; }

  T &operator*() noexcept { return *ptr; }
  const T &operator*() const noexcept { return *ptr; }
  T *operator->() noexcept { return ptr; }
  const T *operator->() const noexcept { return ptr; }
};

/// Naive unique ptr array specialization.
///
/// If a custom deleter is not supplied then we default to the default deleter
/// for the type T[] which is `delete[] ptr`.
///
/// If a custom deleter is supplied, it is wrapped into a functor which in turn
/// is inherited from and called in the destructor.
///
/// A specialization must accept the base template's default argument(s).
template <typename T, typename D>
class naive_unique_ptr<T[], D>
    : std::conditional_t<is_deleter_function_candidate_v<D>,
                         custom_deleter_wrapper<T>, D> {
  using deleter_type = std::conditional_t<is_deleter_function_candidate_v<D>,
                                          custom_deleter_wrapper<T>, D>;
  T *ptr = nullptr;

public:
  naive_unique_ptr() = default;
  naive_unique_ptr(T *p) : ptr{p} {}
  naive_unique_ptr(T *p, void (*pf)(T *)) : deleter_type{pf}, ptr{p} {}

  naive_unique_ptr(const naive_unique_ptr &) = delete;
  naive_unique_ptr &operator=(const naive_unique_ptr &) = delete;

  void swap(naive_unique_ptr &other) noexcept { std::swap(ptr, other.ptr); }

  naive_unique_ptr(naive_unique_ptr &&other) noexcept {
    std::swap(ptr, other.ptr);
  }
  naive_unique_ptr &operator=(naive_unique_ptr &&other) noexcept {
    // just delegate this to our swap by swaping other with a tmp object
    naive_unique_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  ~naive_unique_ptr() { (*static_cast<deleter_type *>(this))(ptr); }

  inline bool empty() const noexcept { return ptr == nullptr; }
  operator bool() const noexcept { return !empty(); }
  bool operator==(const naive_unique_ptr &other) const noexcept {
    return ptr == other.ptr;
  }

  // inferred since C++20
  // bool operator!=(const naive_unique_ptr &other) const noexcept {
  //   return !(*this == other);
  // }

  T *get() noexcept { return ptr; }
  const T *get() const noexcept { return ptr; }

  T &operator[](std::size_t n) noexcept { return ptr[n]; }
  const T &operator[](std::size_t n) const noexcept { return ptr[n]; }
};

}; // namespace NaiveUniquePointerShit

struct B {
  double boo0;
  int boo1;
  float boo2;
};

struct A {
  A(B *b) : b{b} {}
  int foo0;
  float fo16;
  double foo2[5];
  B *b{nullptr}; // needs to be cleaned!
};

static void A_deleter(A *p) {
  if (p->b)
    delete p->b;
  delete p;
  std::cout << "A_deleter called" << std::endl;
}

static void A_invalid_deleter() {
  std::cout
      << "oops! how did this get called? you should never see this message!"
      << std::endl;
}

int main() {
  using namespace NaiveUniquePointerShit;
  naive_unique_ptr<int> int_ptr{new int(10)};
  // naive_unique_ptr<int> up_copy(up);   //<-- this should NOT compile!
  // naive_unique_ptr<int> up_copy;
  // up_copy = up;                        //<-- this should NOT compile!
  // int_ptr[5] = 255;                    //<-- this should NOT compile!
  *int_ptr = 50;
  naive_unique_ptr<int[]> int_array_ptr{new int[50]()};
  int_array_ptr[10] = 50;
  // *int_array_ptr = 100;                //<-- this should NOT compile!
  std::cout << "sizeof naive_unique_ptr<int[]>: " << sizeof(int_array_ptr)
            << std::boolalpha << " == sizeof(*int)? "
            << (sizeof(int_array_ptr) == sizeof(int *)) << std::endl;

  // Custom deleter is needed here otherwise we leak!
  naive_unique_ptr<A, decltype(&A_deleter)> struct_ptr{new A(new B()),
                                                       &A_deleter};
  // This WILL LEAK! Hence why we need a custom deleter!
  naive_unique_ptr<A> leaky_struct_ptr{new A(new B())};

  // For custom pointers we pay the price for an additional function pointer
  // (for the deleter)
  std::cout << "sizeof naive_unique_ptr<A, decltype(&A_deleter)>: "
            << sizeof(struct_ptr) << std::boolalpha << " == sizeof(A*)? "
            << (sizeof(struct_ptr) == sizeof(A *)) << std::endl;

  std::unique_ptr<A, decltype(&A_deleter)> actual_smart_ptr{new A(new B()),
                                                            &A_deleter};

  // Even the std::unique_ptr has to store the pointer somewhere to the deleter
  // somewhere hence why it also pays the additional memory price of a pointer.
  std::cout << "sizeof std::unique_ptr<A, decltype(&A_deleter)>: "
            << sizeof(actual_smart_ptr) << std::boolalpha << " == sizeof(A*)? "
            << (sizeof(actual_smart_ptr) == sizeof(A *)) << std::endl;

  // The smart pointers implemented by std are just so much more polished.
  // I never thought coding a std::unique_ptr clone is so difficult. Let alone
  // a std::shared_ptr

  // naive_unique_ptr<A, decltype(&A_invalid_deleter)> struct_ptr_invalid{new
  // A(new B()), A_invalid_deleter};    //<-- this should NOT compile!
  return 0;
}
