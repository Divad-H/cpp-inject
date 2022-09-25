#pragma once

#include <memory>
#include <type_traits>
#include <vector>

namespace CppInject {
template <typename T, typename Enable = void>
struct IsSharedPointer : std::false_type {};

template <typename T>
struct IsSharedPointer<
    T, typename std::enable_if_t<std::is_same_v<
           typename std::decay_t<T>,
           std::shared_ptr<typename std::decay_t<T>::element_type>>>>
    : std::true_type {};

template <typename T, typename Enable = void>
struct IsVector : std::false_type {};

template <typename T>
struct IsVector<T, typename std::enable_if_t<std::is_same<
                       typename std::decay_t<T>,
                       std::vector<typename T::value_type,
                                   typename T::allocator_type>>::value>>
    : std::true_type {};
}  // namespace CppInject
