// { dg-do run { target c++23 } }

#include <span>
#include <string>
#include <testsuite_allocator.h>
#include <testsuite_hooks.h>
#include <testsuite_iterators.h>

template<typename Range, typename Alloc>
constexpr void
do_test()
{
  // The vector's value_type.
  using V = typename std::allocator_traits<Alloc>::value_type;
  using CT = std::char_traits<V>;

  // The range's value_type.
  using T = std::ranges::range_value_t<Range>;
  T a[]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};

  auto eq = [](const std::basic_string<V, CT, Alloc>& l, std::span<T> r) {
    if (l.size() != r.size())
      return false;
    for (auto i = 0u; i < l.size(); ++i)
      if (l[i] != r[i])
	return false;
    return true;
  };

  Range r4(a, a+4);
  Range r5(a+4, a+9);
  Range r11(a+9, a+20);

  std::basic_string<V, CT, Alloc> v;
  v.append_range(r4);
  VERIFY( eq(v, {a, 4}) );
  v.append_range(r5);
  VERIFY( eq(v, {a, 9}) );

  std::basic_string<V, CT, Alloc> const s = v;
  v.append_range(r11);
  VERIFY( eq(v, a) );
  v.append_range(Range(a, a));
  VERIFY( eq(v, a) );
  v.clear();
  v.append_range(Range(a, a));
  VERIFY( v.empty() );
}

template<typename Range>
constexpr void
do_test_a()
{
  do_test<Range, std::allocator<char>>();
  do_test<Range, __gnu_test::SimpleAllocator<char>>();
  do_test<Range, std::allocator<wchar_t>>();
  do_test<Range, __gnu_test::SimpleAllocator<wchar_t>>();
}

constexpr bool
test_ranges()
{
  using namespace __gnu_test;

  do_test_a<test_forward_range<char>>();
  do_test_a<test_forward_sized_range<char>>();
  do_test_a<test_sized_range_sized_sent<char, forward_iterator_wrapper>>();

  do_test_a<test_input_range<char>>();
  do_test_a<test_input_sized_range<char>>();
  do_test_a<test_sized_range_sized_sent<char, input_iterator_wrapper>>();

  do_test_a<test_range<char, input_iterator_wrapper_nocopy>>();
  do_test_a<test_sized_range<char, input_iterator_wrapper_nocopy>>();
  do_test_a<test_sized_range_sized_sent<char, input_iterator_wrapper_nocopy>>();

  // Not lvalue-convertible to char
  struct C {
    constexpr C(char v) : val(v) { }
    constexpr operator char() && { return val; }
    constexpr bool operator==(char b) const { return b == val; }
    char val;
  };
  using rvalue_input_range = test_range<C, input_iterator_wrapper_rval>;
  do_test<rvalue_input_range, std::allocator<char>>();

  return true;
}

void
test_overlapping()
{
  std::string const s = "1234abcd";

  std::string c = s;
  c.append_range(std::string_view(c));
  VERIFY( c == "1234abcd1234abcd" );

  c = s;
  c.append_range(std::string_view(c).substr(4, 4));
  VERIFY( c == "1234abcdabcd" );

  c = s;
  c.reserve(12);
  c.append_range(std::string_view(c).substr(0, 4));
  VERIFY( c == "1234abcd1234" );
}

int main()
{
  test_ranges();
  test_overlapping();
#if _GLIBCXX_USE_CXX11_ABI
  static_assert( test_ranges() );
#endif // _GLIBCXX_USE_CXX11_ABI
}
