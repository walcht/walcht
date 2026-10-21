#include <memory>

/*
 * Useful for arrays not small enough for stack-based allocation and with a
 * pre-determined fixed size (i.e., std::vector without the overhead of
 * everything needed to deal with resizing).
 */
template <typename T> class fixed_size_array {
public:
  fixed_size_array(size_t count)
      : m_count{count}, m_buff{std::make_unique<T[]>(count)} {}

  fixed_size_array(const fixed_size_array &) = delete;
  fixed_size_array &operator=(const fixed_size_array &) = delete;

  auto size() const { return m_count; }
  bool is_empty() const { return size() == 0; }

  T &operator[](size_t n) { return m_buff[n]; }
  const T &operator[](size_t n) const { return m_buff[n]; }

protected:
  const std::unique_ptr<T[]> m_buff;
  const size_t m_count;
};

int main() {}
