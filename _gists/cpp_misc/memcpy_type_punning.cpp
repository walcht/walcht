#include <cstring>
#include <iostream>

int main() {
  static_assert(sizeof(int) == sizeof(float));
  float f{1.5f};
  int value;
  std::memcpy(&value, &f, sizeof(f));
  std::cout << "value=" << value << std::endl;
  return 0;
}
