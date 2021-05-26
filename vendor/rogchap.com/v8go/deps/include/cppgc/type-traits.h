// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_CPPGC_TYPE_TRAITS_H_
#define INCLUDE_CPPGC_TYPE_TRAITS_H_

// This file should stay with minimal dependencies to allow embedder to check
// against Oilpan types without including any other parts.
#include <type_traits>

namespace cppgc {

class Visitor;

namespace internal {
template <typename T, typename WeaknessTag, typename WriteBarrierPolicy,
          typename CheckingPolicy>
class BasicMember;
struct DijkstraWriteBarrierPolicy;
struct NoWriteBarrierPolicy;
class StrongMemberTag;
class UntracedMemberTag;
class WeakMemberTag;

// Pre-C++17 custom implementation of std::void_t.
template <typename... Ts>
struct make_void {
  typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

// Not supposed to be specialized by the user.
template <typename T>
struct IsWeak : std::false_type {};

// IsTraceMethodConst is used to verify that all Trace methods are marked as
// const. It is equivalent to IsTraceable but for a non-const object.
template <typename T, typename = void>
struct IsTraceMethodConst : std::false_type {};

template <typename T>
struct IsTraceMethodConst<T, void_t<decltype(std::declval<const T>().Trace(
                                 std::declval<Visitor*>()))>> : std::true_type {
};

template <typename T, typename = void>
struct IsTraceable : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct IsTraceable<
    T, void_t<decltype(std::declval<T>().Trace(std::declval<Visitor*>()))>>
    : std::true_type {
  // All Trace methods should be marked as const. If an object of type
  // 'T' is traceable then any object of type 'const T' should also
  // be traceable.
  static_assert(IsTraceMethodConst<T>(),
                "Trace methods should be marked as const.");
};

template <typename T>
constexpr bool IsTraceableV = IsTraceable<T>::value;

template <typename T, typename = void>
struct HasGarbageCollectedMixinTypeMarker : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct HasGarbageCollectedMixinTypeMarker<
    T,
    void_t<typename std::remove_const_t<T>::IsGarbageCollectedMixinTypeMarker>>
    : std::true_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T, typename = void>
struct HasGarbageCollectedTypeMarker : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct HasGarbageCollectedTypeMarker<
    T, void_t<typename std::remove_const_t<T>::IsGarbageCollectedTypeMarker>>
    : std::true_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T, bool = HasGarbageCollectedTypeMarker<T>::value,
          bool = HasGarbageCollectedMixinTypeMarker<T>::value>
struct IsGarbageCollectedMixinType : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct IsGarbageCollectedMixinType<T, false, true> : std::true_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T, bool = HasGarbageCollectedTypeMarker<T>::value>
struct IsGarbageCollectedType : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct IsGarbageCollectedType<T, true> : std::true_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct IsGarbageCollectedOrMixinType
    : std::integral_constant<bool, IsGarbageCollectedType<T>::value ||
                                       IsGarbageCollectedMixinType<T>::value> {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T, bool = (HasGarbageCollectedTypeMarker<T>::value &&
                              HasGarbageCollectedMixinTypeMarker<T>::value)>
struct IsGarbageCollectedWithMixinType : std::false_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename T>
struct IsGarbageCollectedWithMixinType<T, true> : std::true_type {
  static_assert(sizeof(T), "T must be fully defined");
};

template <typename BasicMemberCandidate, typename WeaknessTag,
          typename WriteBarrierPolicy>
struct IsSubclassOfBasicMemberTemplate {
 private:
  template <typename T, typename CheckingPolicy>
  static std::true_type SubclassCheck(
      BasicMember<T, WeaknessTag, WriteBarrierPolicy, CheckingPolicy>*);
  static std::false_type SubclassCheck(...);

 public:
  static constexpr bool value =
      decltype(SubclassCheck(std::declval<BasicMemberCandidate*>()))::value;
};

template <typename T,
          bool = IsSubclassOfBasicMemberTemplate<
              T, StrongMemberTag, DijkstraWriteBarrierPolicy>::value>
struct IsMemberType : std::false_type {};

template <typename T>
struct IsMemberType<T, true> : std::true_type {};

template <typename T, bool = IsSubclassOfBasicMemberTemplate<
                          T, WeakMemberTag, DijkstraWriteBarrierPolicy>::value>
struct IsWeakMemberType : std::false_type {};

template <typename T>
struct IsWeakMemberType<T, true> : std::true_type {};

template <typename T, bool = IsSubclassOfBasicMemberTemplate<
                          T, UntracedMemberTag, NoWriteBarrierPolicy>::value>
struct IsUntracedMemberType : std::false_type {};

template <typename T>
struct IsUntracedMemberType<T, true> : std::true_type {};

}  // namespace internal

/**
 * Value is true for types that inherit from `GarbageCollectedMixin` but not
 * `GarbageCollected<T>` (i.e., they are free mixins), and false otherwise.
 */
template <typename T>
constexpr bool IsGarbageCollectedMixinTypeV =
    internal::IsGarbageCollectedMixinType<T>::value;

/**
 * Value is true for types that inherit from `GarbageCollected<T>`, and false
 * otherwise.
 */
template <typename T>
constexpr bool IsGarbageCollectedTypeV =
    internal::IsGarbageCollectedType<T>::value;

/**
 * Value is true for types that inherit from either `GarbageCollected<T>` or
 * `GarbageCollectedMixin`, and false otherwise.
 */
template <typename T>
constexpr bool IsGarbageCollectedOrMixinTypeV =
    internal::IsGarbageCollectedOrMixinType<T>::value;

/**
 * Value is true for types that inherit from `GarbageCollected<T>` and
 * `GarbageCollectedMixin`, and false otherwise.
 */
template <typename T>
constexpr bool IsGarbageCollectedWithMixinTypeV =
    internal::IsGarbageCollectedWithMixinType<T>::value;

/**
 * Value is true for types of type `Member<T>`, and false otherwise.
 */
template <typename T>
constexpr bool IsMemberTypeV = internal::IsMemberType<T>::value;

/**
 * Value is true for types of type `UntracedMember<T>`, and false otherwise.
 */
template <typename T>
constexpr bool IsUntracedMemberTypeV = internal::IsUntracedMemberType<T>::value;

/**
 * Value is true for types of type `WeakMember<T>`, and false otherwise.
 */
template <typename T>
constexpr bool IsWeakMemberTypeV = internal::IsWeakMemberType<T>::value;

/**
 * Value is true for types that are considered weak references, and false
 * otherwise.
 */
template <typename T>
constexpr bool IsWeakV = internal::IsWeak<T>::value;

}  // namespace cppgc

#endif  // INCLUDE_CPPGC_TYPE_TRAITS_H_
