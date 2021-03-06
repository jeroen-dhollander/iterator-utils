#pragma once

#ifndef _CPP_ITERATORS_ITERATORS_H_
#define _CPP_ITERATORS_ITERATORS_H_

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace iterators {
template <typename> class Chained;
template <typename> class ForwardChained;
template <typename> class Enumerated;
template <typename> class ForwardEnumerated;
template <typename> class Iterated;
template <typename> class ForwardIterated;
template <typename> class ReferencedUnique;
template <typename> class ForwardReferencedUnique;
template <typename> class Referenced;
template <typename> class ForwardReferenced;
template <typename> class Reversed;
template <typename, typename> class Joined;
template <typename, typename> class ForwardJoined;
template <typename, typename> class Mapped;
template <typename, typename> class ForwardMapped;
template <typename, typename> class Filtered;
template <typename, typename> class ForwardFiltered;
template <typename, typename> class Zipped;
template <typename, typename> class ForwardZipped;

namespace details {

// These are defined in C++14/C++17 but not in C++11
template <bool B, class T, class F> using conditional_t = typename std::conditional<B, T, F>::type;
template <typename...> struct conjunction : std::false_type {};
template <typename B1> struct conjunction<B1> : B1 {};
template <typename B1, class... Bn>
struct conjunction<B1, Bn...> : conditional_t<bool(B1::value), conjunction<Bn...>, std::false_type> {};
template <bool B, typename T = void> using enable_if_t = typename std::enable_if<B, T>::type;
template <typename T> using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T> using remove_reference_t = typename std::remove_reference<T>::type;
template <typename T> using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T> struct remove_cvref { typedef remove_cv_t<remove_reference_t<T>> type; };
template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T> struct is_unique_pointer_helper : std::false_type {};
template <typename T> struct is_unique_pointer_helper<std::unique_ptr<T>> : std::true_type {};
template <typename T> struct is_unique_pointer : is_unique_pointer_helper<remove_cv_t<T>> {};

// Returns true if T is a collection over unique_ptr, e.g. vector<std::unique_ptr<int>>
template <typename T>
struct is_unique_pointer_collection : is_unique_pointer<typename remove_cvref_t<T>::value_type> {};

// From https://stackoverflow.com/a/257382/3490116 , adapted to use decltype/declval
template <typename T> class has_rbegin {
  typedef char one;
  typedef long two;

  template <typename C> static one test(decltype(std::declval<remove_cvref_t<C>>().rbegin())*);
  template <typename C> static two test(...);

 public:
  enum { value = sizeof(test<T>(nullptr)) == sizeof(one) };
};
template <typename T> class has_size {
  typedef char one;
  typedef long two;

  template <typename C> static one test(decltype(std::declval<remove_cvref_t<C>>().size())*);
  template <typename C> static two test(...);

 public:
  enum { value = sizeof(test<T>(nullptr)) == sizeof(one) };
};

template <typename T1, typename T2> using have_size = conjunction<has_size<T1>, has_size<T2>>;

// True if both the outer and the nested collection support size
template <typename T> using outer_and_inner_support_size = have_size<T, typename remove_cvref_t<T>::value_type>;

// True if 'T' is a collection that supports forward and reverse iterating
template <typename T> using is_bidirectional_collection = has_rbegin<T>;

template <typename T1, typename T2>
using are_bidirectional_collections = conjunction<is_bidirectional_collection<T1>, is_bidirectional_collection<T2>>;

// True if both the outer and the nested collection are a bidirectional collection
template <typename T>
using is_nested_bidirectional_collection = are_bidirectional_collections<T, typename remove_cvref_t<T>::value_type>;

}  // namespace details

// Allows you to enumerate over the elements of a given collection.
//
// This means that iterating over the returned object will return
// iterator objects with 2 methods:
//    - Postion(): the position of the element in the collection
//    - Value(): a reference to the value of the element
//
// For example, this:
//
// vector<char> values {{'A', 'B', 'C'}};
// for (const auto & item: Enumerate(values))
//     printf("%u: %c\n", item.Position(), item.Value());
//
// will print
//     0: A
//     1: B
//     2: C
template <typename T, typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto Enumerate(T&& iterable) {
  return Enumerated<T>{std::forward<T>(iterable)};
}
template <typename T, typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto Enumerate(T&& iterable) {
  return ForwardEnumerated<T>{std::forward<T>(iterable)};
}

// Allows you to return an iterator over a collection, without actually having to expose access to the collection
// itself.
// Simply write this:
//
// class MyClass
// {
//     public:
//          auto IterateStuff() const
//          {
//              return Iterate(stuff);
//          }
//
//          auto IterateStuff()
//          {
//              return Iterate(stuff);
//          }
//
//     private:
//         MyCollection stuff_;
// };
template <typename T, typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto Iterate(T&& iterable) {
  return Iterated<T>{std::forward<T>(iterable)};
}
template <typename T, typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto Iterate(T&& iterable) {
  return ForwardIterated<T>{std::forward<T>(iterable)};
}

// Allows you to chain a collection of collection,
// e.g. to collect all the elements of a vector of lists
// Simply write this:
//
// vector<list<int>> collection
// for (const int & value : Chain(collection))
// {
//      // do-something
// }
template <typename T, typename details::enable_if_t<details::is_nested_bidirectional_collection<T>::value, int> = 1>
auto Chain(T&& iterable) {
  return Chained<T>{std::forward<T>(iterable)};
}

template <typename T, typename details::enable_if_t<!details::is_nested_bidirectional_collection<T>::value, int> = 1>
auto Chain(T&& iterable) {
  return ForwardChained<T>{std::forward<T>(iterable)};
}

// Allows you to iterate over a collection of pointers using references
//
// This is a nice way to hide how you internally store your objects.
//
// Simply write this:
//
// vector<MyObject*> collection_of_pointers
// for (const MyObject & object : AsReferences(collection_of_pointers))
// {
//      // do-something
// }
template <typename T, typename details::enable_if_t<!details::is_unique_pointer_collection<T>::value, int> = 1,
          typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto AsReferences(T&& iterable) {
  return Referenced<T>{std::forward<T>(iterable)};
}
template <typename T, typename details::enable_if_t<!details::is_unique_pointer_collection<T>::value, int> = 1,
          typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto AsReferences(T&& iterable) {
  return ForwardReferenced<T>{std::forward<T>(iterable)};
}

// Allows you to iterate over a collection of unique_ptr's using references
//
// This is a nice way to hide how you internally store your objects.
//
// Simply write this:
//
// vector<std::unique_ptr<MyObject>> collection_of_unique_pointers
// for (const MyObject & object : AsReferences(collection_of_unique_pointers))
// {
//      // do-something
// }
template <typename T, typename details::enable_if_t<details::is_unique_pointer_collection<T>::value, int> = 1,
          typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto AsReferences(T&& iterable) {
  return ReferencedUnique<T>{std::forward<T>(iterable)};
}
template <typename T, typename details::enable_if_t<details::is_unique_pointer_collection<T>::value, int> = 1,
          typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto AsReferences(T&& iterable) {
  return ForwardReferencedUnique<T>{std::forward<T>(iterable)};
}

// Allows you to iterate back-to-front over a collection
//
// Simply write this:
//
// for (auto & element : Reverse(my_collection))
// {
//      // do-something
//
template <typename T> auto Reverse(T&& iterable) {
  static_assert(details::is_bidirectional_collection<T>::value,
                "Can only reverse collections that have bidirectional iterators");
  return Reversed<T>{std::forward<T>(iterable)};
}

// Allows you to iterate over 2 collections.
// First we'll walk the elements of the first collection,
// then the ones of the second.
//
// Simply write this:
//
// for (auto & element : Joined(collection_1, collection_2))
// {
//      // do-something
// }
//
template <typename T1, typename T2,
          typename details::enable_if_t<details::are_bidirectional_collections<T1, T2>::value, int> = 1>
auto Join(T1&& iterable_1, T2&& iterable_2) {
  return Joined<T1, T2>{std::forward<T1>(iterable_1), std::forward<T2>(iterable_2)};
}
template <typename T1, typename T2,
          typename details::enable_if_t<!details::are_bidirectional_collections<T1, T2>::value, int> = 1>
auto Join(T1&& iterable_1, T2&& iterable_2) {
  return ForwardJoined<T1, T2>{std::forward<T1>(iterable_1), std::forward<T2>(iterable_2)};
}

// Applies the mapping-function to all the items in the input list.
// the mapping-function must take a reference _Iterable::value_type as input.
//
// Simply write this:
//
// std::list<int> collection = ...;
// for (std::string string_value : Map(collection, [](const auto & int_value) { return std::to_string(int_value); }))
// {
//       // do-something
// }
//
template <typename T, typename Function,
          typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto Map(T&& data, Function mapping_function) {
  return Mapped<T, Function>(std::forward<T>(data), std::forward<Function>(mapping_function));
}
template <typename T, typename Function,
          typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto Map(T&& data, Function mapping_function) {
  return ForwardMapped<T, Function>(std::forward<T>(data), std::forward<Function>(mapping_function));
}

// Iterates over the keys of a std::map
template <typename T> auto MapKeys(T&& map) {
  return Map(std::forward<T>(map), [](const auto& map_pair) { return map_pair.first; });
}

// Iterates over the values of a std::map
template <typename T> auto MapValues(T&& map) {
  return Map(std::forward<T>(map), [](const auto& map_pair) { return map_pair.second; });
}

// Returns an iterator over the elements for which filter(element) returns 'true'
template <typename T, typename FilterFunction,
          typename details::enable_if_t<details::is_bidirectional_collection<T>::value, int> = 1>
auto Filter(T&& data, FilterFunction filter) {
  return Filtered<T, FilterFunction>(std::forward<T>(data), std::forward<FilterFunction>(filter));
}
template <typename T, typename FilterFunction,
          typename details::enable_if_t<!details::is_bidirectional_collection<T>::value, int> = 1>
auto Filter(T&& data, FilterFunction filter) {
  return ForwardFiltered<T, FilterFunction>(std::forward<T>(data), std::forward<FilterFunction>(filter));
}

// Creates tuples of the elements in both collections.
//
// Iteration stops as soon as one collection is exhausted,
// so the size is the size of the shortest collection.
//
// For example, this:
//
// vector<int> first {{1, 2, 3, 4, 5};
// vector<char> second {{'A', 'B', 'C'}};
// for (const auto & pair: Zip(first, second))
//     printf("%u: %c\n", item.First(), item.Second());
//
// will print
//     0: A
//     1: B
//     2: C
template <typename T1, typename T2,
          typename details::enable_if_t<details::are_bidirectional_collections<T1, T2>::value, int> = 1>
auto Zip(T1&& iterable_1, T2&& iterable_2) {
  return Zipped<T1, T2>{std::forward<T1>(iterable_1), std::forward<T2>(iterable_2)};
}
template <typename T1, typename T2,
          typename details::enable_if_t<!details::are_bidirectional_collections<T1, T2>::value, int> = 1>
auto Zip(T1&& iterable_1, T2&& iterable_2) {
  return ForwardZipped<T1, T2>{std::forward<T1>(iterable_1), std::forward<T2>(iterable_2)};
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

namespace details {

// These are defined in C++14/C++17 but not in C++11
template <typename...> struct disjunction : std::false_type {};
template <typename B1> struct disjunction<B1> : B1 {};
template <typename B1, class... Bn>
struct disjunction<B1, Bn...> : conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};
template <typename _Container> constexpr auto cbegin(const _Container& container) { return std::begin(container); }
template <typename _Container> constexpr auto cend(const _Container& container) { return std::end(container); }
template <typename _Container> constexpr auto rbegin(_Container& container) { return container.rbegin(); }
template <typename _Container> constexpr auto rbegin(const _Container& container) { return container.rbegin(); }
template <typename _Container> constexpr auto rend(_Container& container) { return container.rend(); }
template <typename _Container> constexpr auto rend(const _Container& container) { return container.rend(); }
template <typename _Container> constexpr auto crbegin(const _Container& container) { return container.rbegin(); }
template <typename _Container> constexpr auto crend(const _Container& container) { return container.rend(); }

// Note: 'std::is_const' is pretty strict, e.g. 'std::is_const<const int&>' returns 'false'.
//       So we're using this construct that checks if it is const in any way
template <typename T> struct is_const_type : std::false_type {};
template <typename T> struct is_const_type<const T> : std::true_type {};
template <typename T> struct is_const_type<const T&> : std::true_type {};
template <typename T> struct is_const_type<const T*> : std::true_type {};
template <typename T> struct is_const_type<T* const> : std::true_type {};
template <typename T> struct is_const_type<const T*&> : std::true_type {};
template <typename T> struct is_const_type<const T&&> : std::true_type {};
template <typename T> struct is_const_type<const T*&&> : std::true_type {};

// True if T1 or T2 is const
template <typename T1, typename T2> using is_any_const = disjunction<is_const_type<T1>, is_const_type<T2>>;

// The collection type is considered const if:
//   - it is 'const T'
//   - its non-const iterator returns a const value
template <typename T>
using is_const_collection =
    disjunction<is_const_type<T>, is_const_type<decltype(*std::declval<typename remove_cvref_t<T>::iterator>())>>;

// Returns T::iterator if T is non_const,
// returns T::const_iterator if it is const.
// e.g. non_const_iterator_t<vector<int>> --> vector<int>::iterator
// e.g. non_const_iterator_t<const vector<int>> --> vector<int>::const_iterator
template <typename T>
using non_const_iterator_t = conditional_t<is_const_type<T>::value, typename remove_cvref_t<T>::const_iterator,
                                           typename remove_cvref_t<T>::iterator>;

// Returns T::reverse_iterator if T is non_const,
// returns T::const_reverse_iterator if it is const.
// e.g. non_const_reverse_iterator_t<vector<int>> --> vector<int>::reverse_iterator
// e.g. non_const_reverse_iterator_t<const vector<int>> --> vector<int>::const_reverse_iterator
template <typename T>
using non_const_reverse_iterator_t =
    conditional_t<is_const_type<T>::value, typename remove_cvref_t<T>::const_reverse_iterator,
                  typename remove_cvref_t<T>::reverse_iterator>;

}  // namespace details

// Adds chained operators to the derived class,
// So you can write Iterate(vector<int>).Map(<map-function>).Reverse().Filter(<filter-function>).Enumerate()
// Note that any operator here consumes 'this' by moving it into the returned collection,
// otherwise doing 'return Iterate(x).Reverse()' would return an instance of Reversed that references an instance of
// Iterate that is freed the second we exit the function.
template <typename DerivedClass> class WithChainedOperators {
 public:
  template <typename Function> auto map(Function&& function) {
    return Map(MoveSelf(), std::forward<Function>(function));
  }

  template <typename Function> auto filter(Function&& function) {
    return Filter(MoveSelf(), std::forward<Function>(function));
  }

  auto reverse() { return Reverse(MoveSelf()); }
  auto enumerate() { return Enumerate(MoveSelf()); }

 private:
  DerivedClass MoveSelf() {
    DerivedClass* self = static_cast<DerivedClass*>(this);
    return std::move(*self);
  }
};

// Adds operators for 'size/Size' and 'empty/IsEmpty' to any class that derives from here.
// The implementation simply forwards the calls to the nested collection.
// 'size' is added only if the nested collection supports 'size'
// (as some STL containers like 'forward_list' do not support it).
//
// The derived class must call 'this->InitializeWithSizeAndEmpty' from their constructor.
template <typename T> class WithSizeAndEmpty {
 public:
  using _collection_type = typename std::remove_reference<T>::type;

  WithSizeAndEmpty() : data_(nullptr) {}

  // STL-container compliant method to check if the container is empty
  bool empty() const { return Data().empty(); }
  // Code style compliant method to check if the container is empty
  bool IsEmpty() const { return Data().empty(); }

  // STL-container compliant method to get the size (if the nested type supports 'size')
  template <typename X = T> details::enable_if_t<details::has_size<X>::value, std::size_t> size() const {
    return Data().size();
  }
  // Code style compliant method to get the size (if the nested type supports 'size')
  template <typename X = T> details::enable_if_t<details::has_size<X>::value, std::size_t> Size() const {
    return Data().size();
  }

 protected:
  void InitializeWithSizeAndEmpty(const _collection_type& data) { data_ = &data; }

 private:
  const _collection_type& Data() const { return *data_; }

  const _collection_type* data_;
};

// Returned value when iterating Enumerate
template <typename T> class Item {
 public:
  Item() : Item(0, nullptr) {}

  Item(int position, T* value) : position_(position), value_(value) {}

  int Position() const { return position_; }
  const T& Value() const { return *value_; }
  T& Value() { return *value_; }

 private:
  int position_;
  T* value_;
};

template <typename _iterator, typename _return_type> class EnumeratedIterator {
 public:
  using _collection_value_type = details::remove_cvref_t<decltype(std::declval<_return_type>().Value())>;
  using _Item = details::remove_cvref_t<_return_type>;

  EnumeratedIterator(_iterator begin, _iterator end, int position, int position_delta)
      : begin_(begin), end_(end), position_(position), position_delta_(position_delta), item_{} {
    SetItem();
  }

  _return_type& operator*() { return item_; }

  _return_type& operator*() const {
    // Dereferencing a const or a non-const iterator should not make a difference
    // (as the constness of the iterator has no bearing on the returned value of the collection).
    // However, as our iterator owns the return-value struct in this particular case,
    // we must cast the const away to achieve this
    return const_cast<_return_type&>(item_);
  }

  void operator++() {
    ++begin_;
    position_ += position_delta_;
    SetItem();
  }

  bool operator==(const EnumeratedIterator& other) const { return begin_ == other.begin_; }
  bool operator!=(const EnumeratedIterator& other) const { return !(*this == other); }

 private:
  void SetItem() {
    if (begin_ != end_)
      item_ = _Item{position_, &NonConstValue()};
  }

  _collection_value_type& NonConstValue() { return const_cast<_collection_value_type&>(*begin_); }

  _iterator begin_;
  _iterator end_;
  int position_;
  int position_delta_;
  _Item item_;
};

// contains the forward iterating shared by all enumerators (forward-only and bidirectional)
template <typename T> class EnumeratedBase : public WithSizeAndEmpty<T> {
 public:
  using _collection_type = typename std::remove_reference<T>::type;
  using _collection_value_type = typename _collection_type::value_type;
  using _collection_const_iterator = typename _collection_type::const_iterator;
  using _collection_iterator = typename _collection_type::iterator;
  using _Item = Item<_collection_value_type>;
  using _non_const_iterator = EnumeratedIterator<_collection_iterator, _Item>;

  using value_type = _Item;
  using const_iterator = EnumeratedIterator<_collection_const_iterator, const _Item>;
  using iterator =
      typename details::conditional_t<details::is_const_collection<T>::value, const_iterator, _non_const_iterator>;

  explicit EnumeratedBase(T&& iterable) : iterable_(std::forward<T>(iterable)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  const_iterator begin() const {
    return const_iterator{details::cbegin(iterable_), details::cend(iterable_), 0, kIncrement};
  }
  const_iterator end() const {
    return const_iterator{details::cend(iterable_), details::cend(iterable_), 0, kIncrement};
  }

  iterator begin() { return iterator{std::begin(iterable_), std::end(iterable_), 0, kIncrement}; }
  iterator end() { return iterator{std::end(iterable_), std::end(iterable_), 0, kIncrement}; }

 protected:
  constexpr static int kIncrement = 1;

  T iterable_;
};

// the forward-only enumerator
template <typename T>
class ForwardEnumerated : public EnumeratedBase<T>, public WithChainedOperators<ForwardEnumerated<T>> {
 public:
  explicit ForwardEnumerated(T&& iterable) : EnumeratedBase<T>(std::forward<T>(iterable)) {}

  // Note: all functionality is proved by the base-classes
};

// the bidirectional enumerator
template <typename T> class Enumerated : public EnumeratedBase<T>, public WithChainedOperators<Enumerated<T>> {
 public:
  using typename EnumeratedBase<T>::_collection_type;
  using typename EnumeratedBase<T>::_collection_value_type;
  using _Item = Item<_collection_value_type>;
  using _collection_const_reverse_iterator = typename _collection_type::const_reverse_iterator;
  using _collection_reverse_iterator = typename _collection_type::reverse_iterator;
  using _non_const_reverse_iterator = EnumeratedIterator<_collection_reverse_iterator, _Item>;

  using const_reverse_iterator = EnumeratedIterator<_collection_const_reverse_iterator, const _Item>;
  using reverse_iterator = typename details::conditional_t<details::is_const_collection<T>::value,
                                                           const_reverse_iterator, _non_const_reverse_iterator>;

  explicit Enumerated(T&& iterable) : EnumeratedBase<T>(std::forward<T>(iterable)) {}

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->iterable_), details::crend(this->iterable_), MaxPosition(),
                                  kDecrement};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->iterable_), details::crend(this->iterable_), MaxPosition(),
                                  kDecrement};
  }

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->iterable_), details::rend(this->iterable_), MaxPosition(),
                            kDecrement};
  }

  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->iterable_), details::rend(this->iterable_), MaxPosition(), kDecrement};
  }

 private:
  int MaxPosition() const { return static_cast<int>(this->size()) - 1; }

  constexpr static int kDecrement = -1;
};

// contains the forward iterating shared by all iterators (forward-only and bidirectional)
template <typename T> class IteratedBase : public WithSizeAndEmpty<T> {
 public:
  using _iterable = typename details::remove_reference_t<T>;

  using value_type = typename _iterable::value_type;
  using const_iterator = typename _iterable::const_iterator;
  using iterator = details::non_const_iterator_t<T>;

  explicit IteratedBase(T&& iterable) : iterable_(std::forward<T>(iterable)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  iterator begin() { return std::begin(iterable_); }
  iterator end() { return std::end(iterable_); }
  const_iterator begin() const { return details::cbegin(iterable_); }
  const_iterator end() const { return details::cend(iterable_); }

 protected:
  T iterable_;
};

// the forward-only iterator
template <typename T> class ForwardIterated : public IteratedBase<T>, public WithChainedOperators<ForwardIterated<T>> {
 public:
  explicit ForwardIterated(T&& iterable) : IteratedBase<T>(std::forward<T>(iterable)) {}

  // Note: all functionality is proved by the base-classes
};

// the bidirectional iterator
template <typename T> class Iterated : public IteratedBase<T>, public WithChainedOperators<Iterated<T>> {
 public:
  using typename IteratedBase<T>::_iterable;
  using const_reverse_iterator = typename _iterable::const_reverse_iterator;
  using reverse_iterator = details::non_const_reverse_iterator_t<T>;

  explicit Iterated(T&& iterable) : IteratedBase<T>(std::forward<T>(iterable)) {}

  reverse_iterator rbegin() { return details::rbegin(this->iterable_); }
  reverse_iterator rend() { return details::rend(this->iterable_); }
  const_reverse_iterator rbegin() const { return details::crbegin(this->iterable_); }
  const_reverse_iterator rend() const { return details::crend(this->iterable_); }
};

template <typename _outer_iterator, typename _inner_iterator> class ChainedIterator {
 public:
  using _inner_iterator_collection = typename _outer_iterator::value_type;
  using GetIteratorFunction = _inner_iterator (*)(_inner_iterator_collection&);

  ChainedIterator(_outer_iterator begin, _outer_iterator end, GetIteratorFunction inner_begin_getter,
                  GetIteratorFunction inner_end_getter)
      : outer_begin_(begin),
        outer_end_(end),
        inner_begin_(),
        inner_end_(),
        get_inner_begin_(inner_begin_getter),
        get_inner_end_(inner_end_getter) {
    this->InitializeInnerCollection();
    SkipEmptyInnerCollections();
  }
  auto& operator*() const { return *inner_begin_; }

  void operator++() {
    ++inner_begin_;
    SkipEmptyInnerCollections();
  }

  bool operator==(const ChainedIterator& other) const { return IsAtEnd() == other.IsAtEnd(); }

  bool operator!=(const ChainedIterator& other) const { return !(*this == other); }

 private:
  bool IsAtEnd() const { return outer_begin_ == outer_end_; }

  bool IsAtEndOfInnerCollection() const { return inner_begin_ == inner_end_; }

  // Initialized 'begin' and 'end' for the inner collection
  void InitializeInnerCollection() {
    if (!IsAtEnd()) {
      inner_begin_ = get_inner_begin_(const_cast<_inner_iterator_collection&>(*outer_begin_));
      inner_end_ = get_inner_end_(const_cast<_inner_iterator_collection&>(*outer_begin_));
    }
  }

  // If we're at the end of the inner collection, advance to the next (non-empty) one
  void SkipEmptyInnerCollections() {
    while (!IsAtEnd() && IsAtEndOfInnerCollection()) {
      AdvanceOuterCollection();
    }
  }

  // Advance to the next inner collection
  void AdvanceOuterCollection() {
    ++outer_begin_;
    InitializeInnerCollection();
  }

  _outer_iterator outer_begin_;
  _outer_iterator outer_end_;
  _inner_iterator inner_begin_;
  _inner_iterator inner_end_;
  GetIteratorFunction get_inner_begin_;
  GetIteratorFunction get_inner_end_;
};

// contains the forward iterating shared by all chained iterators (forward-only and bidirectional)
template <typename T> class ChainedBase {
 public:
  using _outer_collection = details::remove_cvref_t<T>;
  using _outer_const_iterator = typename _outer_collection::const_iterator;
  using _outer_non_const_iterator = typename _outer_collection::iterator;
  using _outer_iterator =
      details::conditional_t<details::is_const_type<T>::value, _outer_const_iterator, _outer_non_const_iterator>;

  using _inner_collection = details::remove_cvref_t<typename _outer_collection::value_type>;
  using _inner_const_iterator = typename _inner_collection::const_iterator;
  using _inner_non_const_iterator = typename _inner_collection::iterator;
  using _inner_iterator =
      details::conditional_t<details::is_const_type<T>::value, _inner_const_iterator, _inner_non_const_iterator>;

  using value_type = typename _inner_collection::value_type;
  using const_iterator = ChainedIterator<_outer_const_iterator, _inner_const_iterator>;
  using iterator = ChainedIterator<_outer_iterator, _inner_iterator>;

  ChainedBase(T&& data) : data_(std::forward<T>(data)) {}

  iterator begin() { return iterator{std::begin(data_), std::end(data_), GetBegin, GetEnd}; }
  iterator end() { return iterator{std::end(data_), std::end(data_), GetBegin, GetEnd}; }

  const_iterator begin() const {
    return const_iterator{details::cbegin(data_), details::cend(data_), GetConstBegin, GetConstEnd};
  }
  const_iterator end() const {
    return const_iterator{details::cend(data_), details::cend(data_), GetConstBegin, GetConstEnd};
  }

  // STL-container compliant method to check if the container is empty
  bool empty() const { return IsEmpty(); }
  // Code style compliant method to check if the container is empty
  bool IsEmpty() const {
    for (const auto& collection : data_) {
      if (!collection.empty())
        return false;
    }
    return true;
  }

  // STL-container compliant method to get the size (if the nested type supports 'size')
  template <typename X = T>
  details::enable_if_t<details::outer_and_inner_support_size<X>::value, std::size_t> size() const {
    return Size();
  }
  // Code style compliant method to get the size (if the nested type supports 'size')
  template <typename X = T>
  details::enable_if_t<details::outer_and_inner_support_size<X>::value, std::size_t> Size() const {
    std::size_t result = 0;
    for (const auto& collection : data_)
      result += collection.size();
    return result;
  }

 protected:
  static _inner_iterator GetBegin(_inner_collection& collection) { return std::begin(collection); }
  static _inner_iterator GetEnd(_inner_collection& collection) { return std::end(collection); }
  static _inner_const_iterator GetConstBegin(_inner_collection& collection) { return details::cbegin(collection); }
  static _inner_const_iterator GetConstEnd(_inner_collection& collection) { return details::cend(collection); }

  T data_;
};

// The forward-only chained iterator
template <typename T> class ForwardChained : public ChainedBase<T>, public WithChainedOperators<ForwardChained<T>> {
 public:
  ForwardChained(T&& data) : ChainedBase<T>(std::forward<T>(data)) {}
  // Note: all functionality is proved by the base-classes
};

// The bi-direcitonal chained iterator
template <typename T> class Chained : public ChainedBase<T>, public WithChainedOperators<Chained<T>> {
 public:
  using typename ChainedBase<T>::_outer_collection;
  using typename ChainedBase<T>::_inner_collection;

  using _outer_const_reverse_iterator = typename _outer_collection::const_reverse_iterator;
  using _outer_non_const_reverse_iterator = typename _outer_collection::reverse_iterator;
  using _outer_reverse_iterator =
      details::conditional_t<details::is_const_type<T>::value, _outer_const_reverse_iterator,
                             _outer_non_const_reverse_iterator>;

  using _inner_const_reverse_iterator = typename _inner_collection::const_reverse_iterator;
  using _inner_non_const_reverse_iterator = typename _inner_collection::reverse_iterator;
  using _inner_reverse_iterator =
      details::conditional_t<details::is_const_type<T>::value, _inner_const_reverse_iterator,
                             _inner_non_const_reverse_iterator>;

  using const_reverse_iterator = ChainedIterator<_outer_const_reverse_iterator, _inner_const_reverse_iterator>;
  using reverse_iterator = ChainedIterator<_outer_reverse_iterator, _inner_reverse_iterator>;

  Chained(T&& data) : ChainedBase<T>(std::forward<T>(data)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->data_), details::rend(this->data_), GetReverseBegin, GetReverseEnd};
  }

  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->data_), details::rend(this->data_), GetReverseBegin, GetReverseEnd};
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->data_), details::crend(this->data_), GetConstReverseBegin,
                                  GetConstReverseEnd};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->data_), details::crend(this->data_), GetConstReverseBegin,
                                  GetConstReverseEnd};
  }

 private:
  static _inner_reverse_iterator GetReverseBegin(_inner_collection& collection) { return details::rbegin(collection); }
  static _inner_reverse_iterator GetReverseEnd(_inner_collection& collection) { return details::rend(collection); }
  static _inner_const_reverse_iterator GetConstReverseBegin(_inner_collection& collection) {
    return details::crbegin(collection);
  }

  static _inner_const_reverse_iterator GetConstReverseEnd(_inner_collection& collection) {
    return details::crend(collection);
  }
};

template <typename _iterator, typename _return_value> class ReferencedIterator {
 public:
  ReferencedIterator(_iterator begin, _iterator end) : begin_(begin), end_(end) {}

  _return_value& operator*() const { return **begin_; }

  void operator++() { ++begin_; }

  bool operator==(const ReferencedIterator& other) const { return begin_ == other.begin_; }
  bool operator!=(const ReferencedIterator& other) const { return !(*this == other); }

 private:
  _iterator begin_;
  _iterator end_;
};

// contains the forward iterating shared by all referenced iterators (forward-only and bidirectional)
template <typename T> class ReferencedBase : public WithSizeAndEmpty<T> {
 public:
  using _collection_type = typename details::remove_reference_t<T>;
  using _collection_value_type = details::remove_pointer_t<typename _collection_type::value_type>;
  using _collection_const_iterator = typename _collection_type::const_iterator;
  using _collection_iterator = typename _collection_type::iterator;

  using value_type = _collection_value_type;
  using const_iterator = ReferencedIterator<_collection_const_iterator, const value_type>;
  using _non_const_iterator = ReferencedIterator<_collection_iterator, value_type>;
  using iterator =
      typename details::conditional_t<details::is_const_type<T>::value, const_iterator, _non_const_iterator>;

  explicit ReferencedBase(T&& iterable) : iterable_(std::forward<T>(iterable)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  iterator begin() { return iterator{std::begin(iterable_), std::end(iterable_)}; }
  iterator end() { return iterator{std::end(iterable_), std::end(iterable_)}; }
  const_iterator begin() const { return const_iterator{details::cbegin(iterable_), details::cend(iterable_)}; }
  const_iterator end() const { return const_iterator{details::cend(iterable_), details::cend(iterable_)}; }

 protected:
  T iterable_;
};

// the forward-only referenced iterator
template <typename T>
class ForwardReferenced : public ReferencedBase<T>, public WithChainedOperators<ForwardReferenced<T>> {
 public:
  explicit ForwardReferenced(T&& iterable) : ReferencedBase<T>(std::forward<T>(iterable)) {}
  // Note: all functionality is proved by the base-classes
};

// the bi-directional referenced iterator
template <typename T> class Referenced : public ReferencedBase<T>, public WithChainedOperators<Referenced<T>> {
 public:
  using typename ReferencedBase<T>::_collection_type;
  using typename ReferencedBase<T>::value_type;
  using _collection_const_reverse_iterator = typename _collection_type::const_reverse_iterator;
  using _collection_reverse_iterator = typename _collection_type::reverse_iterator;

  using const_reverse_iterator = ReferencedIterator<_collection_const_reverse_iterator, const value_type>;
  using _non_const_reverse_iterator = ReferencedIterator<_collection_reverse_iterator, value_type>;
  using reverse_iterator = typename details::conditional_t<details::is_const_type<T>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;
  explicit Referenced(T&& iterable) : ReferencedBase<T>(std::forward<T>(iterable)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->iterable_), details::rend(this->iterable_)};
  }

  reverse_iterator rend() { return reverse_iterator{details::rend(this->iterable_), details::rend(this->iterable_)}; }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->iterable_), details::crend(this->iterable_)};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->iterable_), details::crend(this->iterable_)};
  }
};

template <typename T, typename details::enable_if_t<!details::is_unique_pointer_collection<T>::value, int>>
auto AsReferences(T&& iterable) -> Referenced<T> {
  return Referenced<T>{std::forward<T>(iterable)};
}

template <typename _iterator, typename _return_value> class ReferencedUniqueIterator {
 public:
  ReferencedUniqueIterator(_iterator begin, _iterator end) : begin_(begin), end_(end) {}

  _return_value& operator*() const {
    auto& unique_pointer = *begin_;
    auto* pointer = unique_pointer.get();
    return *pointer;
  }

  void operator++() { ++begin_; }

  bool operator==(const ReferencedUniqueIterator& other) const { return begin_ == other.begin_; }
  bool operator!=(const ReferencedUniqueIterator& other) const { return !(*this == other); }

 private:
  _iterator begin_;
  _iterator end_;
};

// contains the forward iterating shared by all referenced iterators (forward-only and bidirectional)
template <typename T> class ReferencedUniqueBase : public WithSizeAndEmpty<T> {
 public:
  using _collection_type = typename details::remove_reference_t<T>;
  using _collection_value_type = typename _collection_type::value_type;
  using _collection_const_iterator = typename _collection_type::const_iterator;
  using _collection_iterator = typename _collection_type::iterator;

  using value_type = typename _collection_value_type::element_type;
  using _non_const_iterator = ReferencedUniqueIterator<_collection_iterator, value_type>;
  using const_iterator = ReferencedUniqueIterator<_collection_const_iterator, const value_type>;
  using iterator =
      typename details::conditional_t<details::is_const_type<T>::value, const_iterator, _non_const_iterator>;

  explicit ReferencedUniqueBase(T&& iterable) : iterable_(std::forward<T>(iterable)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  iterator begin() { return iterator{std::begin(iterable_), std::end(iterable_)}; }
  iterator end() { return iterator{std::end(iterable_), std::end(iterable_)}; }
  const_iterator begin() const { return const_iterator{details::cbegin(iterable_), details::cend(iterable_)}; }
  const_iterator end() const { return const_iterator{details::cend(iterable_), details::cend(iterable_)}; }

 protected:
  T iterable_;
};

// The forward-only referenced iterator
template <typename T>
class ForwardReferencedUnique : public ReferencedUniqueBase<T>,
                                public WithChainedOperators<ForwardReferencedUnique<T>> {
 public:
  explicit ForwardReferencedUnique(T&& iterable) : ReferencedUniqueBase<T>(std::forward<T>(iterable)) {}
  // Note: all functionality is proved by the base-classes
};

// The bi-directional referenced iterator
template <typename T>
class ReferencedUnique : public ReferencedUniqueBase<T>, public WithChainedOperators<ReferencedUnique<T>> {
 public:
  using typename ReferencedUniqueBase<T>::_collection_type;
  using typename ReferencedUniqueBase<T>::value_type;
  using _collection_const_reverse_iterator = typename _collection_type::const_reverse_iterator;
  using _collection_reverse_iterator = typename _collection_type::reverse_iterator;

  using _non_const_reverse_iterator = ReferencedUniqueIterator<_collection_reverse_iterator, value_type>;
  using const_reverse_iterator = ReferencedUniqueIterator<_collection_const_reverse_iterator, const value_type>;
  using reverse_iterator = typename details::conditional_t<details::is_const_type<T>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;

  explicit ReferencedUnique(T&& iterable) : ReferencedUniqueBase<T>(std::forward<T>(iterable)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->iterable_), details::rend(this->iterable_)};
  }

  reverse_iterator rend() { return reverse_iterator{details::rend(this->iterable_), details::rend(this->iterable_)}; }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->iterable_), details::crend(this->iterable_)};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->iterable_), details::crend(this->iterable_)};
  }
};

template <typename T> class Reversed : public WithChainedOperators<Reversed<T>>, public WithSizeAndEmpty<T> {
 public:
  using _iterable = typename details::remove_reference_t<T>;

  using value_type = typename _iterable::value_type;
  using const_iterator = typename _iterable::const_reverse_iterator;
  using iterator = details::non_const_reverse_iterator_t<T>;
  using const_reverse_iterator = typename _iterable::const_iterator;
  using reverse_iterator = details::non_const_iterator_t<T>;

  explicit Reversed(T&& iterable) : iterable_(std::forward<T>(iterable)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  iterator begin() { return details::rbegin(iterable_); }
  iterator end() { return details::rend(iterable_); }
  const_iterator begin() const { return details::crbegin(iterable_); }
  const_iterator end() const { return details::crend(iterable_); }
  reverse_iterator rbegin() { return std::begin(iterable_); }
  reverse_iterator rend() { return std::end(iterable_); }
  const_reverse_iterator rbegin() const { return details::cbegin(iterable_); }
  const_reverse_iterator rend() const { return details::cend(iterable_); }

 private:
  T iterable_;
};

// The iterator used by 'Join'
template <typename _FirstIterator, typename _SecondIterator> class JoinedIterator {
 public:
  JoinedIterator(_FirstIterator first, _FirstIterator first_end, _SecondIterator second, _SecondIterator second_end)
      : first_(first), first_end_(first_end), second_(second), second_end_(second_end) {}
  auto& operator*() const {
    if (first_ != first_end_)
      return *first_;
    return *second_;
  }

  void operator++() {
    if (first_ != first_end_)
      ++first_;
    else
      ++second_;
  }

  bool operator==(const JoinedIterator& other) const { return first_ == other.first_ && second_ == other.second_; }
  bool operator!=(const JoinedIterator& other) const { return !(*this == other); }

 private:
  _FirstIterator first_;
  _FirstIterator first_end_;
  _SecondIterator second_;
  _SecondIterator second_end_;
};

// contains the forward iterating shared by all joined iterators (forward-only and bidirectional
template <typename T1, typename T2> class JoinedBase {
 public:
  using _iterable_1 = typename details::remove_reference_t<T1>;
  using _const_iterator_1 = typename _iterable_1::const_iterator;
  using _iterator_1 = typename _iterable_1::iterator;

  using _iterable_2 = typename details::remove_reference_t<T2>;
  using _const_iterator_2 = typename _iterable_2::const_iterator;
  using _iterator_2 = typename _iterable_2::iterator;

  static_assert((std::is_same<typename _iterable_1::value_type, typename _iterable_2::value_type>::value),
                "value_type must be same type for both collections");

  using _non_const_iterator = JoinedIterator<typename _iterable_1::iterator, typename _iterable_2::iterator>;

  using value_type = typename _iterable_1::value_type;
  using const_iterator = JoinedIterator<typename _iterable_1::const_iterator, typename _iterable_2::const_iterator>;
  using iterator =
      typename details::conditional_t<details::is_any_const<T1, T2>::value, const_iterator, _non_const_iterator>;

  JoinedBase(T1&& data_1, T2&& data_2) : first_(std::forward<T1>(data_1)), second_(std::forward<T2>(data_2)) {}

  iterator begin() { return iterator{std::begin(first_), std::end(first_), std::begin(second_), std::end(second_)}; }
  iterator end() { return iterator{std::end(first_), std::end(first_), std::end(second_), std::end(second_)}; }

  const_iterator begin() const {
    return const_iterator{details::cbegin(first_), details::cend(first_), details::cbegin(second_),
                          details::cend(second_)};
  }
  const_iterator end() const {
    return const_iterator{details::cend(first_), details::cend(first_), details::cend(second_), details::cend(second_)};
  }

  // STL-container compliant method to check if the container is empty
  bool empty() const { return first_.empty() && second_.empty(); }
  // Code style compliant method to check if the container is empty
  bool IsEmpty() const { return first_.empty() && second_.empty(); }

  template <typename X1 = T1, typename X2 = T2>
  details::enable_if_t<details::have_size<X1, X2>::value, std::size_t> size() const {
    return first_.size() + second_.size();
  }
  template <typename X1 = T1, typename X2 = T2>
  details::enable_if_t<details::have_size<X1, X2>::value, std::size_t> Size() const {
    return first_.size() + second_.size();
  }

 protected:
  T1 first_;
  T2 second_;
};

// The forward-only joined iterator
template <typename T1, typename T2>
class ForwardJoined : public JoinedBase<T1, T2>, public WithChainedOperators<ForwardJoined<T1, T2>> {
 public:
  ForwardJoined(T1&& data_1, T2&& data_2) : JoinedBase<T1, T2>(std::forward<T1>(data_1), std::forward<T2>(data_2)) {}
  // Note: all functionality is proved by the base-classes
};

// The bidirectional joined iterator
template <typename T1, typename T2>
class Joined : public JoinedBase<T1, T2>, public WithChainedOperators<Joined<T1, T2>> {
 public:
  using typename JoinedBase<T1, T2>::_iterable_1;
  using typename JoinedBase<T1, T2>::_iterable_2;
  using _const_reverse_iterator_1 = typename _iterable_1::const_reverse_iterator;
  using _reverse_iterator_1 = typename _iterable_1::reverse_iterator;

  using _const_reverse_iterator_2 = typename _iterable_2::const_reverse_iterator;
  using _reverse_iterator_2 = typename _iterable_2::reverse_iterator;

  using _non_const_reverse_iterator =
      JoinedIterator<typename _iterable_2::reverse_iterator, typename _iterable_1::reverse_iterator>;
  using const_reverse_iterator =
      JoinedIterator<typename _iterable_2::const_reverse_iterator, typename _iterable_1::const_reverse_iterator>;
  using reverse_iterator = typename details::conditional_t<details::is_any_const<T1, T2>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;
  Joined(T1&& data_1, T2&& data_2) : JoinedBase<T1, T2>(std::forward<T1>(data_1), std::forward<T2>(data_2)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->second_), details::rend(this->second_), details::rbegin(this->first_),
                            details::rend(this->first_)};
  }

  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->second_), details::rend(this->second_), details::rend(this->first_),
                            details::rend(this->first_)};
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->second_), details::crend(this->second_),
                                  details::crbegin(this->first_), details::crend(this->first_)};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->second_), details::crend(this->second_),
                                  details::crend(this->first_), details::crend(this->first_)};
  }
};

// the iterator used by 'Map'
template <typename _iterable, typename _function> class MappedIterator {
 public:
  MappedIterator(_iterable begin, _iterable end, const _function& mapping_function)
      : begin_(begin), end_(end), mapping_function_(mapping_function) {}

  auto operator*() const { return mapping_function_(*begin_); }

  void operator++() { ++begin_; }

  bool operator==(const MappedIterator& other) const { return begin_ == other.begin_; }
  bool operator!=(const MappedIterator& other) const { return !(*this == other); }

 private:
  _iterable begin_;
  _iterable end_;
  const _function& mapping_function_;
};

// contains the forward iterating shared by all mapped iterators (forward-only and bidirectional)
template <typename T, typename Function> class MappedBase : public WithSizeAndEmpty<T> {
 public:
  using _iterable = typename details::remove_reference_t<T>;
  using _iterable_value_type = typename _iterable::value_type;
  using _iterable_const_iterator = typename _iterable::const_iterator;
  using _iterable_iterator = typename _iterable::iterator;
  using _non_const_iterator = MappedIterator<_iterable_iterator, typename details::remove_reference_t<Function>>;

  using value_type = typename std::result_of<Function(_iterable_value_type&)>::type;
  using const_iterator = MappedIterator<_iterable_const_iterator, typename details::remove_reference_t<Function>>;
  using iterator =
      typename details::conditional_t<details::is_const_type<T>::value, const_iterator, _non_const_iterator>;

  MappedBase(T&& iterable, Function&& mapping_function)
      : iterable_(std::forward<T>(iterable)), mapping_function_(std::forward<Function>(mapping_function)) {
    this->InitializeWithSizeAndEmpty(iterable_);
  }

  iterator begin() { return iterator{std::begin(iterable_), std::end(iterable_), mapping_function_}; }

  iterator end() { return iterator{std::end(iterable_), std::end(iterable_), mapping_function_}; }

  const_iterator begin() const {
    return const_iterator{details::cbegin(iterable_), details::cend(iterable_), mapping_function_};
  }

  const_iterator end() const {
    return const_iterator{details::cend(iterable_), details::cend(iterable_), mapping_function_};
  }

 protected:
  T iterable_;
  Function mapping_function_;
};

// The forward-only mapped iterator
template <typename T, typename Function>
class ForwardMapped : public MappedBase<T, Function>, public WithChainedOperators<ForwardMapped<T, Function>> {
 public:
  ForwardMapped(T&& iterable, Function&& mapping_function)
      : MappedBase<T, Function>(std::forward<T>(iterable), std::forward<Function>(mapping_function)) {}
  // Note: all functionality is proved by the base-classes
};

// The bi-directional mapped iterator
template <typename T, typename Function>
class Mapped : public MappedBase<T, Function>, public WithChainedOperators<Mapped<T, Function>> {
 public:
  using typename MappedBase<T, Function>::_iterable;
  using _iterable_const_reverse_iterator = typename _iterable::const_reverse_iterator;
  using _iterable_reverse_iterator = typename _iterable::reverse_iterator;
  using _non_const_reverse_iterator =
      MappedIterator<_iterable_reverse_iterator, typename details::remove_reference_t<Function>>;

  using const_reverse_iterator =
      MappedIterator<_iterable_const_reverse_iterator, typename details::remove_reference_t<Function>>;
  using reverse_iterator = typename details::conditional_t<details::is_const_type<T>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;

  Mapped(T&& iterable, Function&& mapping_function)
      : MappedBase<T, Function>(std::forward<T>(iterable), std::forward<Function>(mapping_function)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->iterable_), details::rend(this->iterable_), this->mapping_function_};
  }

  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->iterable_), details::rend(this->iterable_), this->mapping_function_};
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->iterable_), details::crend(this->iterable_),
                                  this->mapping_function_};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->iterable_), details::crend(this->iterable_),
                                  this->mapping_function_};
  }
};

// The iterator used by 'Filter'
template <typename _iterable, typename _function> class FilterIterator {
 public:
  using _return_type = typename std::result_of<decltype (&_iterable::operator*)(_iterable)>::type;

  FilterIterator(_iterable begin, _iterable end, const _function& filter) : begin_(begin), end_(end), filter_(filter) {
    SkipFilteredEntries();
  }

  _return_type operator*() const { return *begin_; }

  void operator++() {
    ++begin_;
    SkipFilteredEntries();
  }

  bool operator==(const FilterIterator& other) const { return begin_ == other.begin_; }
  bool operator!=(const FilterIterator& other) const { return !(*this == other); }

 private:
  bool IsEnd() const { return begin_ == end_; }

  bool IsFiltered() const { return !filter_(*begin_); }

  void SkipFilteredEntries() {
    while (!IsEnd() && IsFiltered())
      ++begin_;
  }

  _iterable begin_;
  _iterable end_;
  const _function& filter_;
};

// contains the forward iterating shared by all filtered iterators (forward-only and bidirectional)
template <typename T, typename FilterFunction> class FilteredBase {
 public:
  using _iterable = typename details::remove_reference_t<T>;
  using _iterable_const_iterator = typename _iterable::const_iterator;
  using _iterable_iterator = typename _iterable::iterator;
  using _non_const_iterator = FilterIterator<_iterable_iterator, typename details::remove_reference_t<FilterFunction>>;

  using value_type = typename _iterable::value_type;
  using const_iterator = FilterIterator<_iterable_const_iterator, typename details::remove_reference_t<FilterFunction>>;
  using iterator =
      typename details::conditional_t<details::is_const_type<T>::value, const_iterator, _non_const_iterator>;

  FilteredBase(T&& iterable, FilterFunction&& filter)
      : iterable_(std::forward<T>(iterable)), filter_(std::forward<FilterFunction>(filter)) {}

  iterator begin() { return iterator{std::begin(iterable_), std::end(iterable_), filter_}; }
  iterator end() { return iterator{std::end(iterable_), std::end(iterable_), filter_}; }
  const_iterator begin() const { return const_iterator{details::cbegin(iterable_), details::cend(iterable_), filter_}; }
  const_iterator end() const { return const_iterator{details::cend(iterable_), details::cend(iterable_), filter_}; }

  // STL-container compliant method to check if the container is empty
  bool empty() const { return IsEmpty(); }
  // Code style compliant method to check if the container is empty
  bool IsEmpty() const {
    for (const auto& value : *this)
      return false;
    return true;
  }

  // STL-container compliant method to get the size (if the nested type supports 'size')
  template <typename X = T> details::enable_if_t<details::has_size<X>::value, std::size_t> size() const {
    return Size();
  }

  // Code style compliant method to get the size (if the nested type supports 'size')
  template <typename X = T> details::enable_if_t<details::has_size<X>::value, std::size_t> Size() const {
    std::size_t result = 0;
    for (const auto& value : *this)
      ++result;
    return result;
  }

 protected:
  T iterable_;
  FilterFunction filter_;
};

// The forward-only filtered iterator
template <typename T, typename FilterFunction>
class ForwardFiltered : public FilteredBase<T, FilterFunction>,
                        public WithChainedOperators<ForwardFiltered<T, FilterFunction>> {
 public:
  ForwardFiltered(T&& iterable, FilterFunction&& filter)
      : FilteredBase<T, FilterFunction>(std::forward<T>(iterable), std::forward<FilterFunction>(filter)) {}
  // Note: all functionality is proved by the base-classes
};

// The bi-directional filtered iterator
template <typename T, typename FilterFunction>
class Filtered : public FilteredBase<T, FilterFunction>, public WithChainedOperators<Filtered<T, FilterFunction>> {
 public:
  using typename FilteredBase<T, FilterFunction>::_iterable;
  using _iterable_const_reverse_iterator = typename _iterable::const_reverse_iterator;
  using _iterable_reverse_iterator = typename _iterable::reverse_iterator;
  using _non_const_reverse_iterator =
      FilterIterator<_iterable_reverse_iterator, typename details::remove_reference_t<FilterFunction>>;

  using const_reverse_iterator =
      FilterIterator<_iterable_const_reverse_iterator, typename details::remove_reference_t<FilterFunction>>;
  using reverse_iterator = typename details::conditional_t<details::is_const_type<T>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;

  Filtered(T&& iterable, FilterFunction&& filter)
      : FilteredBase<T, FilterFunction>(std::forward<T>(iterable), std::forward<FilterFunction>(filter)) {}

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->iterable_), details::rend(this->iterable_), this->filter_};
  }

  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->iterable_), details::rend(this->iterable_), this->filter_};
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->iterable_), details::crend(this->iterable_), this->filter_};
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->iterable_), details::crend(this->iterable_), this->filter_};
  }
};

// Returned value when iterating Zip
template <typename T1, typename T2> class ZippedValue {
 public:
  ZippedValue() : ZippedValue(nullptr, nullptr) {}

  ZippedValue(T1* first, T2* second) : first_(first), second_(second) {}

  const T1& First() const { return *first_; }
  T1& First() { return *first_; }
  const T2& Second() const { return *second_; }
  T2& Second() { return *second_; }

 private:
  T1* first_;
  T2* second_;
};

// The iterator used by 'Zip'
template <typename _first_iterator, typename _second_iterator, typename _return_type> class ZippedIterator {
 public:
  using _first_value_type = details::remove_cvref_t<decltype(std::declval<_return_type>().First())>;
  using _second_value_type = details::remove_cvref_t<decltype(std::declval<_return_type>().Second())>;
  using _ZippedValue = details::remove_cvref_t<_return_type>;

  ZippedIterator(_first_iterator first_begin, _first_iterator first_end, _second_iterator second_begin,
                 _second_iterator second_end)
      : first_begin_(first_begin),
        first_end_(first_end),
        second_begin_(second_begin),
        second_end_(second_end),
        value_{} {
    SetValue();
  }

  _return_type& operator*() { return value_; }

  _return_type& operator*() const {
    // Dereferencing a const or a non-const iterator should not make a difference
    // (as the constness of the iterator has no bearing on the returned value of the collection).
    return const_cast<_return_type&>(value_);
  }

  void operator++() {
    ++first_begin_;
    ++second_begin_;
    SetValue();
  }

  bool operator==(const ZippedIterator& other) const { return IsAtEnd() == other.IsAtEnd(); }
  bool operator!=(const ZippedIterator& other) const { return !(*this == other); }

 private:
  bool IsAtEnd() const { return (first_begin_ == first_end_ || second_begin_ == second_end_); }

  void SetValue() {
    if (!IsAtEnd())
      value_ = _ZippedValue{&NonConstFirstValue(), &NonConstSecondValue()};
  }

  _first_value_type& NonConstFirstValue() { return const_cast<_first_value_type&>(*first_begin_); }
  _second_value_type& NonConstSecondValue() { return const_cast<_second_value_type&>(*second_begin_); }

  _first_iterator first_begin_;
  _first_iterator first_end_;
  _second_iterator second_begin_;
  _second_iterator second_end_;
  _ZippedValue value_;
};

// contains the forward iterating shared by all zipped iterators (forward-only and bidirectional)
template <typename T1, typename T2> class ZippedBase {
 public:
  using _first_collection_type = typename std::remove_reference<T1>::type;
  using _first_collection_value_type = typename _first_collection_type::value_type;
  using _first_collection_const_iterator = typename _first_collection_type::const_iterator;
  using _first_collection_iterator = typename _first_collection_type::iterator;
  using _second_collection_type = typename std::remove_reference<T2>::type;
  using _second_collection_value_type = typename _second_collection_type::value_type;
  using _second_collection_const_iterator = typename _second_collection_type::const_iterator;
  using _second_collection_iterator = typename _second_collection_type::iterator;

  using _Value = ZippedValue<_first_collection_value_type, _second_collection_value_type>;
  using _non_const_iterator = ZippedIterator<_first_collection_iterator, _second_collection_iterator, _Value>;

  using value_type = _Value;
  using const_iterator =
      ZippedIterator<_first_collection_const_iterator, _second_collection_const_iterator, const _Value>;
  using iterator =
      typename details::conditional_t<details::is_any_const<T1, T2>::value, const_iterator, _non_const_iterator>;

  ZippedBase(T1&& first, T2&& second) : first_(std::forward<T1>(first)), second_(std::forward<T2>(second)){};

  const_iterator begin() const {
    return const_iterator{details::cbegin(first_), details::cend(first_), details::cbegin(second_),
                          details::cend(second_)};
  }
  const_iterator end() const {
    return const_iterator{details::cend(first_), details::cend(first_), details::cend(second_), details::cend(second_)};
  }

  iterator begin() { return iterator{std::begin(first_), std::end(first_), std::begin(second_), std::end(second_)}; }
  iterator end() { return iterator{std::end(first_), std::end(first_), std::end(second_), std::end(second_)}; }

  // STL-container compliant method to check if the container is empty
  bool empty() const { return first_.empty() || second_.empty(); }
  // Code style compliant method to check if the container is empty
  bool IsEmpty() const { return first_.empty() || second_.empty(); }

  template <typename X1 = T1, typename X2 = T2>
  details::enable_if_t<details::have_size<X1, X2>::value, std::size_t> size() const {
    return std::min(first_.size(), second_.size());
  }
  template <typename X1 = T1, typename X2 = T2>
  details::enable_if_t<details::have_size<X1, X2>::value, std::size_t> Size() const {
    return std::min(first_.size(), second_.size());
  }

 protected:
  T1 first_;
  T2 second_;
};

// the forward-only version
template <typename T1, typename T2>
class ForwardZipped : public ZippedBase<T1, T2>, public WithChainedOperators<ForwardZipped<T1, T2>> {
 public:
  explicit ForwardZipped(T1&& first, T2&& second)
      : ZippedBase<T1, T2>(std::forward<T1>(first), std::forward<T2>(second)) {}

  // Note: all functionality is proved by the base-classes
};

// the bidirectional version
template <typename T1, typename T2>
class Zipped : public ZippedBase<T1, T2>, public WithChainedOperators<Zipped<T1, T2>> {
 public:
  using typename ZippedBase<T1, T2>::_first_collection_type;
  using typename ZippedBase<T1, T2>::_first_collection_value_type;
  using _first_collection_const_reverse_iterator = typename _first_collection_type::const_reverse_iterator;
  using _first_collection_reverse_iterator = typename _first_collection_type::reverse_iterator;

  using typename ZippedBase<T1, T2>::_second_collection_type;
  using typename ZippedBase<T1, T2>::_second_collection_value_type;
  using _second_collection_const_reverse_iterator = typename _second_collection_type::const_reverse_iterator;
  using _second_collection_reverse_iterator = typename _second_collection_type::reverse_iterator;

  using typename ZippedBase<T1, T2>::_Value;
  using _non_const_reverse_iterator =
      ZippedIterator<_first_collection_reverse_iterator, _second_collection_reverse_iterator, _Value>;

  using const_reverse_iterator =
      ZippedIterator<_first_collection_const_reverse_iterator, _second_collection_const_reverse_iterator, const _Value>;
  using reverse_iterator = typename details::conditional_t<details::is_any_const<T1, T2>::value, const_reverse_iterator,
                                                           _non_const_reverse_iterator>;

  explicit Zipped(T1&& first, T2&& second) : ZippedBase<T1, T2>(std::forward<T1>(first), std::forward<T2>(second)) {}

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator{details::crbegin(this->first_), details::crend(this->first_),
                                  details::crbegin(this->second_), details::crend(this->second_)};
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator{details::crend(this->first_), details::crend(this->first_),
                                  details::crend(this->second_), details::crend(this->second_)};
  }

  reverse_iterator rbegin() {
    return reverse_iterator{details::rbegin(this->first_), details::rend(this->first_), details::rbegin(this->second_),
                            details::rend(this->second_)};
  }
  reverse_iterator rend() {
    return reverse_iterator{details::rend(this->first_), details::rend(this->first_), details::rend(this->second_),
                            details::rend(this->second_)};
  }
};
}  // namespace iterators

#endif  // _CPP_ITERATORS_ITERATORS_H_
