#include <iostream>
#include <string>
#include <string_view>

class VkHandleWrapper {
private:
  int m_some_shit{0};
  std::string m_name{};

public:
  // default constructor (deleted)
  VkHandleWrapper() = delete;

  // move constructor
  VkHandleWrapper(VkHandleWrapper &&other)
      : m_name{other.m_name}, m_some_shit{other.m_some_shit} {}

  // custom contructor
  VkHandleWrapper(std::string_view name, int val = 0)
      : m_name{name}, m_some_shit{val} {
    std::cout << __PRETTY_FUNCTION__ << " for: " << m_name << "\n";
  }

  // another custom constructor (nullptr constructor)
  VkHandleWrapper(std::nullptr_t) : m_name{"nullptr"} {
    std::cout << __PRETTY_FUNCTION__ << " for: " << m_name << "\n";
  }

  // copy constructor
  VkHandleWrapper(const VkHandleWrapper &) = delete;
  VkHandleWrapper &operator=(const VkHandleWrapper &) = delete;

  // move assignment
  VkHandleWrapper &operator=(VkHandleWrapper &&rls) {
    std::cout << __PRETTY_FUNCTION__ << " for: " << m_name << "\n";
    return *this;
  }

  ~VkHandleWrapper() {
    std::cout << __PRETTY_FUNCTION__ << " for: " << m_name << "\n";
    clear(); // cleanup of resources
  }

  // this is just to simulate resources cleanup
  void clear() {}

  std::string get_name() { return m_name; }
};

// pass by value - to figure out what happens when a user-defined object is
// passed by value
void test_func(VkHandleWrapper some_a) {
  std::cout << "scope exit of: " << __PRETTY_FUNCTION__ << "\n";
}

int main() {
  // although you see the '=' assigment character there is actually no
  // invokation of an assigment operator here. All that is invoked is the right
  // constructor - in this case, the default constructor.
  //
  // this is another (albeit silly) reason to use the {} initialization syntax
  // to avoid confusion (especially for beginner C++ users)
  std::cout
      << "constructed a via default constructor (auto a = VkHandleWrapper())"
      << "\n";
  auto a = VkHandleWrapper("a");

  std::cout << "constructed a via default constructor (VkHandleWrapper a2{})"
            << "\n";
  VkHandleWrapper a2{"a2"}; // this should invoke same constructor as above

  // VulkanHpp makes a lot of use of this
  std::cout << "assigned a nullptr (a = nullptr)" << "\n";
  a = nullptr; // this should call the destructor and cleanup the underlying
               // resources

  // this should invoke the copy constructor and errors  at compile time
  // auto a_copy = VkHandleWrapper(a);

  test_func(a);

  std::cout << "going out of scope (program exit)" << "\n";
}
