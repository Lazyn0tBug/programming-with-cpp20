// Copyright (c) Andreas Fertig.
// SPDX-License-Identifier: MIT

#include <version>

#include <algorithm>
#include <cstddef>
#include <span>

#if(__cpp_nontype_template_parameter_class >= 201806) &&             \
  (__cpp_nontype_template_args >= 201911) &&                         \
  not defined(_MSC_VER) &&                                           \
  not(defined(__GNUC__) && !defined(__clang__))

template<typename CharT, std::size_t N>
struct fixed_string {
  CharT data[N]{};

  constexpr fixed_string(const CharT (&str)[N])
  {
    std::copy_n(str, N, data);
  }
};

template<fixed_string Str>  // #A Takes the fixed string as NTTP
struct FormatString {
  static constexpr auto fmt =
    Str;  // #B Store the string for easy access

  // #C Use ranges to count all the percent signs.
  static constexpr auto numArgs = std::ranges::count(fmt.data, '%');

  // #D For usability provide a conversion operator
  operator const auto *() const { return fmt.data; }
};

template<fixed_string Str>
constexpr auto operator"" _fs()
{
  return FormatString<Str>{};
}

template<typename T, typename U>
constexpr bool
  plain_same_v =  // #A Helper type-trait to strip the type down
  std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template<typename T>
constexpr static bool match(const char c)
{
  switch(c) {  // #B Our specifier to type mapping table
    case 'd': return plain_same_v<int, T>;
    case 'c': return plain_same_v<char, T>;
    case 'f': return plain_same_v<double, T>;
    case 's':  // #C Character strings are a bit more difficult
      return (plain_same_v<char, std::remove_all_extents_t<T>> &&
              std::is_array_v<T>) ||
             (plain_same_v<char*, std::remove_all_extents_t<T>> &&
              std::is_pointer_v<T>);
    default: return false;
  }
}

template<int I, typename CharT>
constexpr auto get(const std::span<const CharT>& str)
{
  auto       start = std::begin(str);
  const auto end   = std::end(str);

  for(int i = 0; i <= I; ++i) {  // #A Do it I-times
    // #B Find the next percent sign
    start = std::ranges::find(start, end, '%');
    ++start;  // #C Without this we see the same percent sign
  }

  return *start;  // #D Return the format specifier character
}

template<typename CharT, typename... Ts>
constexpr bool IsMatching(std::span<const CharT> str)
{
  return [&]<size_t... I>(std::index_sequence<I...>)
  {
    return ((match<Ts>(get<I>(str))) && ...);
  }
  (std::make_index_sequence<sizeof...(Ts)>{});
}

template<typename... Ts>
void print(auto fmt, const Ts&... ts)
{
  // #A Use the count of arguments and compare it to the size of  the
  // pack
  static_assert(fmt.numArgs == sizeof...(Ts));

  // #B Check that specifier matches type
  static_assert(
    IsMatching<std::decay_t<decltype(
                 fmt.fmt.data[0])>,  // #C Get the  underlying type
               Ts...                 // #D Expand the types pack
               >(fmt.fmt.data));

  printf(fmt, ts...);
}

template<typename... Ts>
void print(char* s, const Ts&... ts)
{
  printf(s, ts...);
}

template<typename... Ts>
constexpr bool always_false_v =
  false;  // #A Helper, returns always false

template<typename... Ts>
void print(const char* s, Ts&&... ts)
{
  // #B Use the helper to trigger the assert whenever this  template
  // is instantiated
  static_assert(always_false_v<Ts...>, "Please use _fs");
}

int main()
{
  print(FormatString<"%s, %s">{}, "Hello", "C++20");

  print("%s, %s"_fs, "Hello", "C++20");

  char fmt[]{"Hello, %s"};
  print(fmt, "C++20");

  // fmt("test");
}
#else
int main()
{
#  pragma message("not supported")
}
#endif