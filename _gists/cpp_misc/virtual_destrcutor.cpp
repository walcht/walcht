/**
 * Showcases why a destructor on a base class should be marked virtual if we
 * intend to manage derived-class objects through a pointer to the base class.
 *
 * compile with:
 * g++ -g virtual_destrcutor.cpp -o virtual_destrcutor -O2 -fsanitize=address
 */

#include <iostream>

class BaseNonVirtual {
public:
  BaseNonVirtual() {
    std::cout << "BaseNonVirtual()" << std::endl;
    m_some_resource = new unsigned char[m_count];
  }

  ~BaseNonVirtual() {
    std::cout << "~BaseNonVirtual()" << std::endl;
    delete[] m_some_resource;
  }

private:
  unsigned char *m_some_resource{nullptr};
  const size_t m_count{5};
};

class DerivedNonVirtual : public BaseNonVirtual {
public:
  DerivedNonVirtual() : BaseNonVirtual() {
    m_some_other_resource = new unsigned char[m_size];
  }

  // this will not be called if we call delete on this object's
  // cast-to-BaseNonTrivial pointer
  ~DerivedNonVirtual() { delete[] m_some_other_resource; }

private:
  unsigned char *m_some_other_resource{nullptr};
  const size_t m_size{30};
};

class BaseVirtual {
public:
  BaseVirtual() {
    std::cout << "BaseVirtual()" << std::endl;
    m_some_resource = new unsigned char[m_count];
  }

  virtual ~BaseVirtual() {
    std::cout << "~BaseVirtual()" << std::endl;
    delete[] m_some_resource;
  }

private:
  unsigned char *m_some_resource{nullptr};
  const size_t m_count{5};
};

class DerivedVirtual : public BaseVirtual {
public:
  DerivedVirtual() : BaseVirtual() {
    m_some_other_resource = new unsigned char[m_size];
  }

  ~DerivedVirtual() /* override is optional */ {
    delete[] m_some_other_resource;
  }

private:
  unsigned char *m_some_other_resource{nullptr};
  const size_t m_size{30};
};

int main() {
  //
  // This will cause the address sanitizer to output something like this:
  //  ==75299==ERROR: AddressSanitizer: new-delete-type-mismatch on 0x503000000040 in thread T0:
  //    object passed to delete has wrong type:
  //    size of the allocated type:   32 bytes;
  //    size of the deallocated type: 16 bytes.
  //
  BaseNonVirtual *derived_non_virtual_dtor = new DerivedNonVirtual;
  delete derived_non_virtual_dtor; // <-- memory leak!

  BaseVirtual *derived_virtual_dtor = new DerivedVirtual;
  delete derived_virtual_dtor; // <-- no leaks
  return 0;
}
