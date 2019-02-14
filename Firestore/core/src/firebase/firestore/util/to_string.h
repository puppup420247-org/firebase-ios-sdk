/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_TO_STRING_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_TO_STRING_H_

#if __OBJC__
#import <Foundation/Foundation.h>
#endif  // __OBJC__

#include <algorithm>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>

#include "Firestore/core/src/firebase/firestore/util/string_apple.h"
#include "Firestore/core/src/firebase/firestore/util/string_format.h"
#include "Firestore/core/src/firebase/firestore/util/type_traits.h"
#include "absl/meta/type_traits.h"
#include "absl/strings/str_join.h"

namespace firebase {
namespace firestore {
namespace util {

template <typename T>
std::string ToString(const T& value);

namespace impl {

// Checks whether the given type `T` defines a member function `ToString`

template <typename T, typename = absl::void_t<>>
struct has_to_string : std::false_type {};

template <typename T>
struct has_to_string<T, absl::void_t<decltype(std::declval<T>().ToString())>>
    : std::true_type {};

template <int I>
struct Choice : Choice<I + 1> {};

template <>
struct Choice<5> {};

#if __OBJC__

// Objective-C class
template <typename T,
          typename = absl::enable_if_t<is_objective_c_pointer<T>::value>>
std::string ToStringImpl(T value, Choice<0>) {
  return MakeString([value description]);
}

#endif  // __OBJC__

// Has `ToString` member function
template <typename T, typename = absl::enable_if_t<has_to_string<T>::value>>
std::string ToStringImpl(const T& value, Choice<1>) {
  return value.ToString();
}

// `std::string`
template <typename T,
          typename = absl::enable_if_t<std::is_same<std::string, T>::value>>
std::string ToStringImpl(const T& value, Choice<2>) {
  return value;
}

// Associative container
template <typename T,
          typename = absl::enable_if_t<is_associative_container<T>::value>>
std::string ToStringImpl(const T& value, Choice<3>) {
  std::string contents = absl::StrJoin(
      value, ", ", [](std::string* out, const typename T::value_type& kv) {
        out->append(
            StringFormat("%s: %s", ToString(kv.first), ToString(kv.second)));
      });
  return std::string{"{"} + contents + "}";  // NOLINT(whitespace/braces)
}

// Container
template <typename T, typename = absl::enable_if_t<is_iterable<T>::value>>
std::string ToStringImpl(const T& value, Choice<4>) {
  std::string contents = absl::StrJoin(
      value, ", ", [](std::string* out, const typename T::value_type& element) {
        out->append(ToString(element));
      });
  return std::string{"["} + contents + "]";  // NOLINT(whitespace/braces)
}

// Fallback
template <typename T>
std::string ToStringImpl(const T& value, Choice<5>) {
  FormatArg arg{value};
  return std::string{arg.data(), arg.data() + arg.size()};
}

}  // namespace impl

/**
 * Creates a human-readable description of the given `value`. The representation
 * is loosely inspired by Python.
 *
 * The general idea is to create the description by using the most specific
 * available function that creates a string representation of the class; for
 * containers, do this recursively, adding some minimal container formatting to
 * the output.
 *
 * Example:
 *
 * std::vector<DocumentKey> v{
 *     DocumentKey({"foo/bar"}),
 *     DocumentKey({"this/that"})
 * };
 * assert(ToString(v) == "[foo/bar, this/that]");
 *
 * std::map<int, std::string> m{
       {1, "foo"},
       {2, "bar"}
 * };
 * assert(ToString(m) == "{1: foo, 2: bar}");
 *
 * The following algorithm is used:
 *
 *  - if `value` defines a member function called `ToString`, the description is
 *    created by invoking the function;
 *
 *  - (in Objective-C++ only) otherwise, if `value` is an Objective-C class, the
 *    description is created by calling `[value description]`and converting the
 *    result to an `std::string`;
 *
 *  - otherwise, if `value` is an `std::string`, it's used as is;
 *
 * - otherwise, if `value` is an associative container (`std::map`,
 *   `std::unordered_map`, `f:f:immutable::SortedMap`, etc.), the description is
 *   of the form:
 *
 *     {key1: value1, key2: value2}
 *
 *    where the description of each key and value is created by running
 *    `ToString` recursively;
 *
 *  - otherwise, if `value` is a container, the description is of the form:
 *
 *      [element1, element2]
 *
 *    where the description of each element is created by running `ToString`
 *    recursively;
 *
 * - otherwise, `std::to_string` is used as a fallback. If `std::to_string` is
 *   not defined for the class, a compilation error will be produced.
 *
 * Implementation notes: to rank different choices and avoid clashes (e.g.,
 * a type that is an associative container is also a (simple) container), tag
 * dispatch is used. Each function in the chain either is tagged by
 * `std::true_type` and can process the value, or is tagged by `std::false_type`
 * and passes the value to the next function by the rank. When passing to the
 * next function, some trait corresponding to the function is given in place of
 * the tag; for example, `StringToString`, which can handle `std::string`s, is
 * invoked with `std::is_same<T, std::string>` as the tag.
 */

template <typename T>
std::string ToString(const T& value) {
  return impl::ToStringImpl(value, impl::Choice<0>{});
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_TO_STRING_H_
