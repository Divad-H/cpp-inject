#pragma once

#include <tuple>
#include <type_traits>

namespace CppInject {
namespace detail {
template <class T>
T& pretendToCreateAnything();

template <typename T, int ArgIndex>
struct OverloadResolutionHelper {
  friend auto& functionWithReturnTypeOfArg(
      OverloadResolutionHelper<T, ArgIndex>);
};

template <typename T, typename TArg, int ArgIndex,
          // do not find copy / move constructor
          typename = typename std::enable_if_t<
              !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                              std::remove_cv_t<std::remove_reference_t<TArg>>>>>
struct FunctionWithReturnTypeOfArgWrapper {
  friend auto& functionWithReturnTypeOfArg(
      OverloadResolutionHelper<T, ArgIndex>) {
    return pretendToCreateAnything<std::remove_cv_t<TArg>>();
  }
};

// The templated conversion operator is enabled for the correct argument using
// SFINAE
template <typename T, int ArgIndex>
struct ToConstructorArgTypeConvertible {
  template <typename TArg,
            int = sizeof(FunctionWithReturnTypeOfArgWrapper<T, TArg, ArgIndex>)>
  operator TArg&();
  template <typename TArg,
            int = sizeof(FunctionWithReturnTypeOfArgWrapper<T, TArg, ArgIndex>)>
  operator TArg&&();
};

template <typename T, int... ArgIndices>
constexpr size_t numberOfConstructorArgsIntern(
    // overload resolution will prefer this when called with 0
    int,
    // uses SFINAE to enable this for the correct number of arguments only
    decltype(T(ToConstructorArgTypeConvertible<T, ArgIndices>{}...))* =
        nullptr) {
  return sizeof...(ArgIndices);
}

template <typename T, int... ArgIndices>
constexpr size_t numberOfConstructorArgsIntern(
    char) {  // overload resolution will disfavor this overload when called with
             // 0
  return numberOfConstructorArgsIntern<T, ArgIndices..., sizeof...(ArgIndices)>(
      0);
}

template <typename T, typename U>
struct ConstructorArgsAsTupleIntern;

template <typename T, int... ArgIndices>
struct ConstructorArgsAsTupleIntern<T,
                                    std::integer_sequence<int, ArgIndices...>> {
  using TupleType =
      std::tuple<std::remove_reference_t<decltype(functionWithReturnTypeOfArg(
          OverloadResolutionHelper<T, ArgIndices>{}))>...>;
};
}  // namespace detail

/// <summary>
/// Get the number of arguments of a constructor
/// </summary>
/// <typeparam name="T">The type of the class with the constructor</typeparam>
/// <returns>The number of arguments of the constructor with the lowest number
/// of arguments</returns>
template <typename T>
constexpr auto numberOfConstructorArgs() {
  return detail::numberOfConstructorArgsIntern<T>(0);
}

/// <summary>
/// Get constructor arguments as a tuple.
/// Note that qualifiers and references are dropped.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
using ConstructorArgsAsTuple = typename detail::ConstructorArgsAsTupleIntern<
    T,
    std::make_integer_sequence<int, numberOfConstructorArgs<T>()>>::TupleType;
}  // namespace CppInject
