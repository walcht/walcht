#include <memory>

class requires_dynamic_alloc {
public:
private:
  ~requires_dynamic_alloc() = default;
  friend struct requires_dynamic_alloc_cleaner;
};

/* functor to clean-up the dynamic-only-allocatable class above */
struct requires_dynamic_alloc_cleaner {
  template <class T> void operator()(T *p) const { delete p; }
};

int main() {

  // requires_dynamic_alloc r0;  // <-- this will not compile

  // this also wont compile:
  /* error: ‘requires_dynamic_alloc::~requires_dynamic_alloc()’ is private
   * within this context */
  // auto p0 = std::unique_ptr<requires_dynamic_alloc>{
  //     std::make_unique<requires_dynamic_alloc>()};

  // ok. This will use operator()(requires_dynamic_alloc*) for cleanup
  auto p1 =
      std::unique_ptr<requires_dynamic_alloc, requires_dynamic_alloc_cleaner>{
          new requires_dynamic_alloc};
}
