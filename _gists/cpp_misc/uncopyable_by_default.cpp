#include <iostream>
#include <memory>
#include <vector>

struct SomeObject {
  int a;
  float b;
  double c;

  SomeObject(int a, float b, double c) : a{a}, b{b}, c{c} {}
  /* etc ... */
};

class ObjectsOwnerAndManager {
public:
  ObjectsOwnerAndManager() {
    m_objs.reserve(3);
    m_objs.emplace_back(std::make_unique<SomeObject>(1, 2, 3));
    m_objs.emplace_back(std::make_unique<SomeObject>(4, 7, 7));
    m_objs.emplace_back(std::make_unique<SomeObject>(5, 7, 7));
  }
  inline void PrintSomeShit() const {
    for (const auto &pObj : m_objs) {
      std::cout << pObj->a << std::endl;
    }
  };

private:
  // This actually doesn't work because initializer lists ALWAYS perform copies
  // and std::unique_ptr's are not copyable
  // std::vector<std::unique_ptr<SomeObject>> m_objs{
  //     std::make_unique<SomeObject>(1, 2, 3),
  //     std::make_unique<SomeObject>(5, 3, 7),
  // };

  std::vector<std::unique_ptr<SomeObject>> m_objs;
};

int main() {
  auto owner_0 = ObjectsOwnerAndManager{};
  owner_0.PrintSomeShit();

  // this fails because m_objs is non-copyable by default
  // auto owner_1 = ObjectsOwnerAndManager(owner_0);
  // owner_1.PrintSomeShit();
}
