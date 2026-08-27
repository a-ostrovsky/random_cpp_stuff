#include <meta>
#include <type_traits>
#include <vector>

namespace StaticUniqueVec {
// Sorted vector, remove adjacent duplicates
template <int... I> struct Vector;

consteval std::meta::info unique_of(std::meta::info t) {
  std::vector<int> out;
  for (auto a : std::meta::template_arguments_of(t)) {
    int v = std::meta::extract<int>(a);
    if (out.empty() || out.back() != v) {
      out.push_back(v);
    }
  }

  std::vector<std::meta::info> newargs{};
  for (int v : out) {
    newargs.push_back(std::meta::reflect_constant(v));
  }
  return std::meta::substitute(^^Vector, newargs);
}

template <typename T> using Unique = typename[:unique_of(^^T):];

static_assert(std::is_same_v<Unique<Vector<1, 1, 2, 3, 3>>, Vector<1, 2, 3>>);
} // namespace StaticUniqueVec
