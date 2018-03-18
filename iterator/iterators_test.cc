
#include "iterators.h"
#include <list>
#include <memory>
#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace iterators {
using std::list;
using std::string;
using std::vector;
using testing::ElementsAre;
using testing::ElementsAreArray;

// Prints a human-readable string of the given type, e.g. 'int const *'.
// https://stackoverflow.com/a/20170989/3490116
template <class T>
std::string type_name() {
  typedef typename std::remove_reference<T>::type TR;
  std::unique_ptr<char, void (*)(void*)> own(
#ifndef _MSC_VER
      abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
#else
      nullptr,
#endif
      std::free);
  std::string r = own != nullptr ? own.get() : typeid(TR).name();
  if (std::is_const<TR>::value)
    r += " const";
  if (std::is_volatile<TR>::value)
    r += " volatile";
  if (std::is_lvalue_reference<T>::value)
    r += "&";
  else if (std::is_rvalue_reference<T>::value)
    r += "&&";
  return r;
}

// Checks that the type returned by '_actual' matches the '_expected' type.
// On failure, prints a human-readable message
#define EXPECT_TYPE(_expected, _actual) EXPECT_EQ(type_name<_expected>(), type_name<_actual>())

// Tests that the non_const iterator has the correct type, and returns values of the expected type
#define TEST_NON_CONST_ITERATOR(_iterable, _expected_type)                                      \
  {                                                                                             \
    using IterableClass = decltype(_iterable);                                                  \
    /* Test begin/end return instances of _iterable::iterator */                                \
    EXPECT_TYPE(IterableClass::iterator, decltype(_iterable.begin()));                          \
    EXPECT_TYPE(IterableClass::iterator, decltype(_iterable.end()));                            \
    /* Test iterator return value */                                                            \
    EXPECT_TYPE(_expected_type, decltype(std::declval<IterableClass::iterator>().operator*())); \
  }

// Tests that the const iterator has the correct type, and returns values of the expected type
#define TEST_CONST_ITERATOR(_iterable, _expected_type)                                                \
  {                                                                                                   \
    using IterableClass = decltype(_iterable);                                                        \
    /* Test begin/end return instances of _iterable::const_iterator */                                \
    EXPECT_TYPE(IterableClass::const_iterator, decltype(std::as_const(_iterable).begin()));           \
    EXPECT_TYPE(IterableClass::const_iterator, decltype(std::as_const(_iterable).end()));             \
    /* Test const_iterator return value */                                                            \
    EXPECT_TYPE(_expected_type, decltype(std::declval<IterableClass::const_iterator>().operator*())); \
  }

list<int> AnyCollection() {
  return list<int>{1, 3, 5};
}

list<int> AnyCollectionReversed() {
  return {5, 3, 1};
}

// Executes '++' on each value in the iterable.
// Used to ensure we can non-const access the elements
template <typename _Iterable>
auto& IncreaseAll(_Iterable* iterable) {
  for (auto& value : *iterable)
    value++;
  return *iterable;
}

// Doubles each value in the iterable.
// Used to ensure we can non-const access the elements
template <typename _Iterable>
auto DoubleAll(_Iterable* iterable) {
  for (auto& value : *iterable)
    value *= 2;
  return *iterable;
}

// Appends the given string to each (string) value in the iterable.
// Used to ensure we can non-const access the elements
template <typename _Iterable>
auto AppendAll(_Iterable* iterable, string suffix) {
  list<std::string> result;
  for (auto& value : *iterable) {
    value += suffix;
    result.push_back(value);
  }
  return result;
}

template <typename _Enumerator>
string FormatEnumerate(const _Enumerator& iterable) {
  string result{};
  for (const auto& item : iterable)
    result += std::to_string(item.Position()) + ": " + item.Value() + ", ";
  return result;
}

TEST(IsConstTest, SanityCheck) {
  // Test that our 'details::is_const_type' utility works as expected
#define IS_CONST(_type) details::is_const_type<_type>::value
  EXPECT_TRUE(IS_CONST(const int&));
  EXPECT_TRUE(IS_CONST(const int));
  EXPECT_TRUE(IS_CONST(const int*));
  EXPECT_TRUE(IS_CONST(int* const));
  EXPECT_TRUE(IS_CONST(const int*&));
  EXPECT_TRUE(IS_CONST(const int&&));
  EXPECT_TRUE(IS_CONST(const int*&&));
  EXPECT_TRUE(IS_CONST(int* const&));
  EXPECT_FALSE(IS_CONST(int&));
  EXPECT_FALSE(IS_CONST(int));
  EXPECT_FALSE(IS_CONST(int*));
  EXPECT_FALSE(IS_CONST(int*&));
  EXPECT_FALSE(IS_CONST(int&&));
  EXPECT_FALSE(IS_CONST(int*&&));
}

TEST(TypeNameTest, SanityCheck) {
  // Test that our 'type_name' works as expected
  EXPECT_EQ("int const&", type_name<const int&>());
  EXPECT_EQ("int const", type_name<const int>());
  EXPECT_EQ("int const *", type_name<const int*>());
  EXPECT_EQ("int * const", type_name<int* const>());
  EXPECT_EQ("int const *&", type_name<const int*&>());
  EXPECT_EQ("int const&&", type_name<const int&&>());
  EXPECT_EQ("int const *&&", type_name<const int*&&>());
  EXPECT_EQ("int * const&", type_name<int* const&>());
  EXPECT_EQ("int&", type_name<int&>());
  EXPECT_EQ("int", type_name<int>());
  EXPECT_EQ("int *", type_name<int*>());
  EXPECT_EQ("int *&", type_name<int*&>());
  EXPECT_EQ("int&&", type_name<int&&>());
  EXPECT_EQ("int *&&", type_name<int*&&>());
}

TEST(NonConstIterator, SanityCheck) {
  EXPECT_TYPE(vector<int>::iterator, details::non_const_iterator_t<vector<int>>);
  EXPECT_TYPE(vector<int>::iterator, details::non_const_iterator_t<vector<int>&>);
  EXPECT_TYPE(vector<int>::iterator, details::non_const_iterator_t<vector<int>&&>);
  EXPECT_TYPE(vector<int>::const_iterator, details::non_const_iterator_t<const vector<int>>);
  EXPECT_TYPE(vector<int>::const_iterator, details::non_const_iterator_t<const vector<int>&>);
}

TEST(NonConstReverseIterator, SanityCheck) {
  EXPECT_TYPE(vector<int>::reverse_iterator, details::non_const_reverse_iterator_t<vector<int>>);
  EXPECT_TYPE(vector<int>::reverse_iterator, details::non_const_reverse_iterator_t<vector<int>&>);
  EXPECT_TYPE(vector<int>::reverse_iterator, details::non_const_reverse_iterator_t<vector<int>&&>);
  EXPECT_TYPE(vector<int>::const_reverse_iterator, details::non_const_reverse_iterator_t<const vector<int>>);
  EXPECT_TYPE(vector<int>::const_reverse_iterator, details::non_const_reverse_iterator_t<const vector<int>&>);
}

TEST(EnumerateTest, ReturnsCorrectValues) {
  vector<char> collection{'A', 'B', 'C'};
  auto iterator = Enumerate(collection);

  EXPECT_EQ("0: A, 1: B, 2: C, ", FormatEnumerate(iterator));
  EXPECT_EQ("0: A, 1: B, 2: C, ", FormatEnumerate(std::as_const(iterator)));
}

TEST(EnumerateTest, CanModifyValues) {
  vector<char> collection{'A', 'B', 'C'};
  auto iterator = Enumerate(collection);

  auto& first = *iterator.begin();
  first.Value() = 'Z';

  EXPECT_THAT(collection, ElementsAre('Z', 'B', 'C'));
}

TEST(EnumerateTest, EnumerateOverNonConstCollection) {
  vector<char> collection{'A', 'B', 'C'};
  auto iterator = Enumerate(collection);

  TEST_NON_CONST_ITERATOR(iterator, Item<char>&);
  TEST_CONST_ITERATOR(iterator, Item<char> const&);
  EXPECT_TYPE(Item<char>, decltype(iterator)::value_type);
}

TEST(EnumerateTest, EnumerateOverConstCollection) {
  // Note: In this case, even iterating non-const uses a const_iterator
  vector<char> collection{'A', 'B', 'C'};
  auto iterator = Enumerate(std::as_const(collection));

  TEST_NON_CONST_ITERATOR(iterator, Item<char> const&);
  TEST_CONST_ITERATOR(iterator, Item<char> const&);
  EXPECT_TYPE(Item<char>, decltype(iterator)::value_type);
}

TEST(EnumerateTest, EnumerateRvalueCollection) {
  vector<char> collection{'A', 'B', 'C'};
  auto iterator = Enumerate(std::move(collection));

  TEST_NON_CONST_ITERATOR(iterator, Item<char>&);
  TEST_CONST_ITERATOR(iterator, Item<char> const&);
  EXPECT_TYPE(Item<char>, decltype(iterator)::value_type);
}

TEST(IterateTest, ReturnsCorrectValues) {
  vector<int> collection{1, 3, 5};
  auto iterator = Iterate(collection);

  EXPECT_THAT(iterator, ElementsAre(1, 3, 5));
  EXPECT_THAT(std::as_const(iterator), ElementsAre(1, 3, 5));
}

TEST(IterateTest, CanModifyValues) {
  vector<int> collection{1, 3, 5};
  auto iterator = Iterate(collection);

  int& first = *iterator.begin();
  first = 123;

  EXPECT_THAT(iterator, ElementsAre(123, 3, 5));
}

TEST(IterateTest, IterateOverNonConstCollection) {
  vector<int> collection{1, 3, 5};
  auto iterator = Iterate(collection);

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(IterateTest, IterateOverConstCollection) {
  // Note: In this case, even iterating non-const uses a const_iterator
  vector<int> collection{1, 3, 5};
  auto iterator = Iterate(std::as_const(collection));

  TEST_NON_CONST_ITERATOR(iterator, int const&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(IterateTest, IterateRvalueCollection) {
  vector<int> collection{1, 3, 5};
  auto iterator = Iterate(std::move(collection));

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(ReverseTest, ReturnsCorrectValues) {
  vector<int> collection{1, 3, 5};
  auto iterator = Reverse(collection);

  EXPECT_THAT(iterator, ElementsAre(5, 3, 1));
  EXPECT_THAT(std::as_const(iterator), ElementsAre(5, 3, 1));
}

TEST(ReverseTest, CanModifyValues) {
  vector<int> collection{1, 3, 5};
  auto iterator = Reverse(collection);

  int& first = *iterator.begin();
  first = 123;

  EXPECT_THAT(iterator, ElementsAre(123, 3, 1));
}

TEST(ReverseTest, ReverseOverNonConstCollection) {
  vector<int> collection{1, 3, 5};
  auto iterator = Reverse(collection);

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(ReverseTest, ReverseOverConstCollection) {
  // Note: In this case, even iterating non-const uses a const_iterator
  vector<int> collection{1, 3, 5};
  auto iterator = Reverse(std::as_const(collection));

  TEST_NON_CONST_ITERATOR(iterator, int const&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(ReverseTest, ReverseRvalueCollection) {
  vector<int> collection{1, 3, 5};
  auto iterator = Reverse(std::move(collection));

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(JoinTest, ReturnsCorrectValues) {
  list<int> first{1, 2, 3};
  vector<int> second{4, 5, 6};
  auto iterator = Join(first, second);

  EXPECT_THAT(iterator, ElementsAre(1, 2, 3, 4, 5, 6));
  EXPECT_THAT(std::as_const(iterator), ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(JoinTest, WorksForEmptyCollections) {
  list<int> empty{};
  vector<int> other{1};

  EXPECT_THAT(Join(empty, other), ElementsAre(1));
  EXPECT_THAT(Join(other, empty), ElementsAre(1));
  EXPECT_THAT(Join(empty, empty), ElementsAre());
}

TEST(JoinTest, CanModifyValues_InBothCollections) {
  list<int> first{1};
  vector<int> second{2};

  auto iterator = Join(first, second);

  for (auto& value : iterator)
    value += 100;

  EXPECT_THAT(iterator, ElementsAre(101, 102));
}

TEST(JoinTest, JoinOverNonConstCollections) {
  list<int> first{};
  vector<int> second{};
  auto iterator = Join(first, second);

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(JoinTest, JoinOverConstCollections) {
  // Note: In this case, even iterating non-const uses a const_iterator
  list<int> first{};
  vector<int> second{};
  auto iterator = Join(std::as_const(first), std::as_const(second));

  TEST_NON_CONST_ITERATOR(iterator, int const&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(JoinTest, JoinOverConstAndNonConstCollections) {
  // Note: In this case, even iterating non-const uses a const_iterator
  list<int> first{};
  vector<int> second{};

  auto iterator_1 = Join(std::as_const(first), second);
  TEST_NON_CONST_ITERATOR(iterator_1, int const&);
  TEST_CONST_ITERATOR(iterator_1, int const&);

  auto iterator_2 = Join(first, std::as_const(second));
  TEST_NON_CONST_ITERATOR(iterator_2, int const&);
  TEST_CONST_ITERATOR(iterator_2, int const&);
}

TEST(JoinTest, JoinRvalueCollections) {
  list<int> first{};
  vector<int> second{};
  auto iterator = Join(std::move(first), std::move(second));

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

std::string ToString(const int& value) {
  return std::to_string(value);
}

TEST(MapTest, ReturnsCorrectValues) {
  vector<int> collection{1, 3, 5};
  auto iterator = Map(collection, ToString);

  EXPECT_THAT(iterator, ElementsAre("1", "3", "5"));
  EXPECT_THAT(std::as_const(iterator), ElementsAre("1", "3", "5"));
}

TEST(MapTest, CanModifyValues) {
  // Note: For map, the non-const version means we send a non-const value into the mapping function
  vector<int> collection{1, 3, 5};
  auto iterator = Map(collection, [](int& value) -> int {
    value += 100;
    return 0;
  });

  for (const int& value : iterator) {
    // simply iterating so the value is updated in the mapping function
  }

  EXPECT_THAT(collection, ElementsAre(101, 103, 105));
}

TEST(MapTest, MapOverNonConstCollection) {
  // For Map, both const and non-const iterators return the same type (i.e. the return value of the mapping-function)
  vector<int> collection{1, 3, 5};
  auto iterator = Map(collection, ToString);

  TEST_NON_CONST_ITERATOR(iterator, std::string);
  TEST_CONST_ITERATOR(iterator, std::string);
  EXPECT_TYPE(std::string, decltype(iterator)::value_type);
}

TEST(MapTest, MapOverConstCollection) {
  // For Map, both const and non-const iterators return the same type (i.e. the return value of the mapping-function)
  vector<int> collection{1, 3, 5};
  auto iterator = Map(std::as_const(collection), ToString);

  TEST_NON_CONST_ITERATOR(iterator, std::string);
  TEST_CONST_ITERATOR(iterator, std::string);
  EXPECT_TYPE(std::string, decltype(iterator)::value_type);
}

TEST(MapTest, MapRvalueCollection) {
  // For Map, both const and non-const iterators return the same type (i.e. the return value of the mapping-function)
  vector<int> collection{1, 3, 5};
  auto iterator = Map(std::move(collection), ToString);

  TEST_NON_CONST_ITERATOR(iterator, std::string);
  TEST_CONST_ITERATOR(iterator, std::string);
  EXPECT_TYPE(std::string, decltype(iterator)::value_type);
}

TEST(MapTest, MapKeys__extract_keys_from_std_map) {
  std::map<string, int> input{{{"a", 1}, {"b", 2}}};

  EXPECT_THAT(MapKeys(input), ElementsAre("a", "b"));
}

TEST(MapTest, MapValues__extract_values_from_std_map) {
  std::map<string, int> input{{{"a", 1}, {"b", 2}}};

  EXPECT_THAT(MapValues(input), ElementsAre(1, 2));
}

static bool is_odd(const int& value) {
  return (value % 2) != 0;
}

TEST(FilterTest, ReturnsCorrectValues) {
  vector<int> collection{1, 2, 3, 4, 5};
  auto iterator = Filter(collection, is_odd);

  EXPECT_THAT(iterator, ElementsAre(1, 3, 5));
  EXPECT_THAT(std::as_const(iterator), ElementsAre(1, 3, 5));
}

TEST(FilterTest, CanFilterFirstValue) {
  vector<int> collection{0, 1};
  auto iterator = Filter(collection, is_odd);

  EXPECT_THAT(iterator, ElementsAre(1));
}

TEST(FilterTest, CanFilterConsecutiveValues) {
  vector<int> collection{0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4};
  auto iterator = Filter(collection, is_odd);

  EXPECT_THAT(iterator, ElementsAre(1, 3));
}

TEST(FilterTest, CanModifyValues) {
  vector<int> collection{1, 2, 3, 4, 5};
  auto iterator = Filter(collection, is_odd);

  int& first = *iterator.begin();
  first = 123;

  EXPECT_THAT(iterator, ElementsAre(123, 3, 5));
}

TEST(FilterTest, FilterOverNonConstCollection) {
  vector<int> collection{1, 2, 3, 4, 5};
  auto iterator = Filter(collection, is_odd);

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(FilterTest, FilterOverConstCollection) {
  // Note: In this case, even iterating non-const uses a const_iterator
  vector<int> collection{1, 2, 3, 4, 5};
  auto iterator = Filter(std::as_const(collection), is_odd);

  TEST_NON_CONST_ITERATOR(iterator, int const&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

TEST(FilterTest, FilterRvalueCollection) {
  vector<int> collection{1, 2, 3, 4, 5};
  auto iterator = Filter(std::move(collection), is_odd);

  TEST_NON_CONST_ITERATOR(iterator, int&);
  TEST_CONST_ITERATOR(iterator, int const&);
  EXPECT_TYPE(int, decltype(iterator)::value_type);
}

std::list<std::unique_ptr<int>> ToUniquePtrList(int* values, int values_size) {
  std::list<std::unique_ptr<int>> result{};
  for (int i = 0; i < values_size; i++)
    result.push_back(std::make_unique<int>(values[i]));
  return result;
}

TEST(AsReferences_unique_ptr, can_const_iterate_const_pointer_collection) {
  int values[] = {1, 3, 5};
  auto input{ToUniquePtrList(values, 3)};

  auto result = AsReferences(std::as_const(input));
  EXPECT_THAT(std::as_const(result), ElementsAreArray(values));
}

TEST(AsReferences_unique_ptr, can_const_iterate_non_const_pointer_collection) {
  int values[] = {1, 3, 5};
  auto input{ToUniquePtrList(values, 3)};

  auto result = AsReferences(input);
  EXPECT_THAT(std::as_const(result), ElementsAreArray(values));
}

TEST(AsReferences_unique_ptr, can_non_const_iterate_non_const_pointer_collection) {
  int values[] = {1, 3, 5};
  auto input{ToUniquePtrList(values, 3)};

  auto result = AsReferences(std::move(input));
  EXPECT_THAT(IncreaseAll(&result), ElementsAre(2, 4, 6));
}

TEST(AsReferences_unique_ptr, can_const_iterate_rvalue_pointer_collection) {
  int values[] = {1, 3, 5};
  auto input{ToUniquePtrList(values, 3)};

  auto result = AsReferences(std::move(input));
  EXPECT_THAT(std::as_const(result), ElementsAreArray(values));
}

TEST(AsReferences_unique_ptr, can_non_const_iterate_rvalue_pointer_collection) {
  int values[] = {1, 3, 5};
  auto input{ToUniquePtrList(values, 3)};

  auto result = AsReferences(std::move(input));
  EXPECT_THAT(IncreaseAll(&result), ElementsAre(2, 4, 6));
}

TEST(ChainTest, can_iterate_non_empty_collections) {
  vector<list<int>> collection{{1, 2, 3}, {4, 5, 6}};

  auto result = Chain(collection);
  EXPECT_THAT(result, ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ChainTest, skips_over_empty_first_collections) {
  vector<list<int>> collection{{}, {}, {1, 2, 3}};

  auto result = Chain(collection);
  EXPECT_THAT(result, ElementsAre(1, 2, 3));
}

TEST(ChainTest, skips_over_empty_collections) {
  vector<list<int>> collection{{1}, {}, {}, {2, 3}, {}};

  auto result = Chain(collection);
  EXPECT_THAT(result, ElementsAre(1, 2, 3));
}

TEST(ChainTest, survives_empty_outer_collection) {
  vector<list<int>> collection{};

  auto result = Chain(collection);
  EXPECT_THAT(result, ElementsAre());
}

TEST(ChainTest, can_iterate_const_collections) {
  vector<list<int>> collection{{1, 2, 3}, {4, 5, 6}};

  auto result = Chain(std::as_const(collection));
  EXPECT_THAT(std::as_const(result), ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ChainTest, can_const_iterate_non_const_collection) {
  vector<list<int>> collection{{1, 2, 3}, {4, 5, 6}};

  auto result = Chain(collection);
  EXPECT_THAT(std::as_const(result), ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ChainTest, can_non_const_iterate_non_const_collection) {
  vector<list<int>> collection{{1, 2, 3}, {4, 5, 6}};

  auto result = Chain(collection);
  EXPECT_THAT(DoubleAll(&result), ElementsAre(2, 4, 6, 8, 10, 12));
}

TEST(ChainTest, can_const_iterate_rvalue_collection) {
  auto result = Chain(vector<list<int>>{{1, 2, 3}, {4, 5, 6}});

  EXPECT_THAT(std::as_const(result), ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ChainTest, can_non_const_iterate_rvalue_collection) {
  auto result = Chain(vector<list<int>>{{1, 2, 3}, {4, 5, 6}});

  EXPECT_THAT(DoubleAll(&result), ElementsAre(2, 4, 6, 8, 10, 12));
}

}  // namespace iterators
