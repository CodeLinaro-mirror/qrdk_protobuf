// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PROTOBUF_TEST_H__
#define GOOGLE_PROTOBUF_PROTOBUF_TEST_H__

// Inline value- and type-parameterized test macros for GoogleTest.
//
// These macros allow tests to parameterize over values and types directly
// inline within the test body, without the boilerplate of `TEST_P` +
// `INSTANTIATE_TEST_SUITE_P` or `TYPED_TEST_SUITE` + `TYPED_TEST`.
// When multiple `PB_TEST_CHOOSE_VALUE` and/or `PB_TEST_CHOOSE_TYPE` statements
// appear in a test body, a separate GoogleTest case is registered and run for
// every combination in their Cartesian product.
//
// Public Macros:
//
//   PB_TEST(TestSuiteName, TestName)
//     Defines a standalone test (analogous to GoogleTest's `TEST`) that
//     supports `PB_TEST_CHOOSE_VALUE` and `PB_TEST_CHOOSE_TYPE` in its body.
//     Can also be used with no `PB_TEST_CHOOSE_*` calls as a normal test.
//
//   PB_TEST_F(TestFixtureName, TestName)
//     Defines a test that uses fixture class `TestFixtureName` (analogous to
//     GoogleTest's `TEST_F`) and supports `PB_TEST_CHOOSE_VALUE` and
//     `PB_TEST_CHOOSE_TYPE` in its body.
//
//   PB_TEST_CHOOSE_VALUE(v1, v2, ...)
//     Evaluates to `const T&` for one of the provided values `{v1, v2, ...}`,
//     which must share a common type `T`. Each lexical call site defines an
//     independent value dimension. Value expressions may depend on types
//     selected by `PB_TEST_CHOOSE_TYPE` in the same test.
//
//   PB_TEST_CHOOSE_TYPE(T1, T2, ...)
//     Evaluates to a type (usable in `using T = PB_TEST_CHOOSE_TYPE(...);` or
//     any type context) for one of the provided types `T1, T2, ...`. Each
//     lexical call site defines an independent type dimension.
//
// Example:
//
//   PB_TEST(MySuite, WorksForAllCombinations) {
//     using T = PB_TEST_CHOOSE_TYPE(int32_t, int64_t, double);
//     T delta = PB_TEST_CHOOSE_VALUE(T{0}, T{1}, T{10});
//     bool negate = PB_TEST_CHOOSE_VALUE(false, true);
//
//     T value = negate ? -delta : delta;
//     EXPECT_EQ(std::abs(value), delta);
//   }
//
//   class MyFixture : public ::testing::Test { ... };
//
//   PB_TEST_F(MyFixture, WorksWithFixture) {
//     using MessageT = PB_TEST_CHOOSE_TYPE(FooProto, BarProto);
//     int count = PB_TEST_CHOOSE_VALUE(1, 5, 10);
//     ...
//   }
//
// Test Naming:
//   Registered test names have the form:
//     `<TestName>/<v1>/<v2>/.../<T1>/<T2>/...`
//   (omitting the value or type suffix when none are present), with value and
//   type names automatically sanitized and disambiguated.
//
// Restrictions:
//   - Do NOT define `PB_TEST` or `PB_TEST_F` in header files.
//   - `PB_TEST_CHOOSE_VALUE` and `PB_TEST_CHOOSE_TYPE` must be written
//     lexically inside the `PB_TEST` / `PB_TEST_F` body (not inside helper
//     functions defined outside the test).
//   - Do NOT place `PB_TEST_CHOOSE_VALUE` inside a runtime loop expecting it to
//     yield different values on each iteration; a single lexical call site
//     always returns the same selected value for a given test run.

#include <deque>
#include <map>
#include <type_traits>

#include <gtest/gtest.h>
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/charset.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/port.h"

#define PB_TEST(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, ::testing::Test)

#define PB_TEST_F(test_suite_name, test_name) \
  PB_TEST_(test_suite_name, test_name, test_suite_name)

#define PB_TEST_CHOOSE_VALUE(...)                                         \
  ::google::protobuf::internal::TestChooseValueSelector<InternalSelf_, __COUNTER__, \
                                              InternalTypeId_...>(        \
      this->pb_test_internal_param_idx_,                                  \
      [] { return ::google::protobuf::internal::ChooseValueImpl({__VA_ARGS__}); })

#define PB_TEST_CHOOSE_TYPE(...)                                            \
  typename ::google::protobuf::internal::TestChooseTypeSelector<                      \
      InternalSelf_, ::google::protobuf::internal::TestChooseTypeParams<__VA_ARGS__>, \
      __COUNTER__, InternalTypeId_...>::type

// =============================================================================
// Implementation details follow. Do not use directly.
// =============================================================================

namespace google::protobuf::internal {

#define PB_TEST_IMPL_(test_suite_name, test_name, self_class, parent_class)   \
  static_assert(sizeof(GTEST_STRINGIFY_(test_suite_name)) > 1,                \
                "test_suite_name must not be empty");                         \
  static_assert(sizeof(GTEST_STRINGIFY_(test_name)) > 1,                      \
                "test_name must not be empty");                               \
  class self_class : public parent_class {                                    \
   public:                                                                    \
    using InternalParent_ = parent_class;                                     \
    using InternalSelf_ = self_class;                                         \
                                                                              \
    self_class() = default;                                                   \
    template <size_t... type_id>                                              \
    explicit self_class(::std::index_sequence<type_id...>, size_t idx)        \
        : pb_test_internal_param_idx_(idx),                                   \
          test_body_(&self_class::TestBodyImpl<type_id...>) {                 \
      static_assert((1, ..., type_id) != 0,                                   \
                    "There should not be a zero at the tail");                \
    }                                                                         \
    ~self_class() override = default;                                         \
    self_class(const self_class&) = delete;                                   \
    void operator=(const self_class&) = delete; /* NOLINT */                  \
    template <size_t... type_id>                                              \
    static auto Instantiate_() {                                              \
      return &self_class::TestBodyImpl<type_id...>;                           \
    }                                                                         \
                                                                              \
   private:                                                                   \
    void TestBody() override { (this->*test_body_)(); }                       \
    template <size_t... InternalTypeId_>                                      \
    void TestBodyImpl();                                                      \
    static bool reg_ [[maybe_unused]];                                        \
    size_t pb_test_internal_param_idx_ = 0;                                   \
    void (self_class::*test_body_)() = &self_class::TestBodyImpl<>;           \
  };                                                                          \
                                                                              \
  bool self_class::reg_ =                                                     \
      (::google::protobuf::internal::PartialTestRegistry::Get().SetPrimary<self_class>( \
           #test_suite_name, #test_name, __FILE__, __LINE__),                 \
       false);                                                                \
  template <size_t... InternalTypeId_>                                        \
  void self_class::TestBodyImpl()

#define PB_TEST_(test_suite_name, test_name, parent_class)                 \
  PB_TEST_IMPL_(test_suite_name, test_name, test_suite_name##_##test_name, \
                parent_class)

class PartialTestRegistry {
 public:
  template <int... n>
  static constexpr const void* kVectorKey = &kVectorKey<n...>;

  static auto& Get() {
    static auto* r = new PartialTestRegistry();
    return *r;
  }

  template <typename Fixture, size_t... n>
  void AddDoRegister() {
    auto& pf = fixtures_[kKey<Fixture>];
    auto& t = pf.per_instantiation_[kVectorKey<n...>];
    t.instantiation_key = {n...};
    t.do_register = [this](size_t i) {
      auto& pf = fixtures_[kKey<Fixture>];
      auto& t = pf.per_instantiation_[kVectorKey<n...>];
      const std::string type_name = t.TypeName({n...});
      const std::string value_name = t.values_.empty() ? "" : t.ValueName(i);
      std::string test_name = pf.test_name;
      if (!value_name.empty()) {
        absl::StrAppend(&test_name, "/", value_name);
      }
      if (!type_name.empty()) {
        absl::StrAppend(&test_name, "/", type_name);
      }

      ::testing::RegisterTest(
          pf.test_suite_name, test_name.c_str(),
          type_name.empty() ? nullptr : type_name.c_str(),
          value_name.empty() ? nullptr : value_name.c_str(), pf.file, pf.line,
          [i]() -> typename Fixture::InternalParent_* {
            return new Fixture(std::index_sequence<n...>{}, i);
          });
    };
  }

  template <typename Fixture>
  void SetPrimary(const char* test_suite_name, const char* test_name,
                  const char* file, int line) {
    auto& pf = fixtures_[kKey<Fixture>];
    pf.test_suite_name = test_suite_name;
    pf.test_name = test_name;
    pf.file = file;
    pf.line = line;
    AddDoRegister<Fixture>();
  }

  static void SanitizeNames(std::vector<std::string>& names) {
    for (auto& name : names) {
      const auto replace = [&](absl::string_view needle,
                               absl::string_view rep) {
        while (true) {
          if (auto pos = absl::string_view(name).find(needle);
              pos != name.npos) {
            name.replace(pos, needle.size(), rep.data(), rep.size());
          } else {
            break;
          }
        }
      };

      // Let's keep "pointer" in the name.
      replace("*", "Ptr");

      for (char& c : name) {
        static constexpr absl::CharSet kAllowed =
            absl::CharSet::AsciiAlphanumerics() | absl::CharSet::Char('_');
        if (!kAllowed.contains(c)) {
          c = '_';
        }
      }

      // Collapse duplicate _
      replace("__", "_");

      // Remove leading _
      while (name[0] == '_') name.erase(0, 1);
      // Remove trailing _
      while (!name.empty() && name.back() == '_') name.pop_back();

      // Don't have empty names.
      if (name.empty()) name = "_";
    }

    DedupNames(names);
  }

  static void DedupNames(std::vector<std::string>& names) {
    absl::flat_hash_set<std::string> dupes;
    for (auto& s : names) {
      if (dupes.insert(s).second) continue;
      for (int i = 2;; ++i) {
        std::string dedup =
            s == "_" ? absl::StrCat("_", i) : absl::StrCat(s, "_", i);
        if (dupes.insert(dedup).second) {
          s = std::move(dedup);
          break;
        }
      }
    }
  }

  template <typename Fixture, typename T>
  void AddValueDimension(int id, const void* instantiation_key,
                         std::deque<T> values) {
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation_[instantiation_key]
                  .values_[id];
    for (size_t i = 0; i < values.size(); ++i) {
      d.names.push_back(MakeName(values[i]));
    }
    SanitizeNames(d.names);
    d.values = new auto(std::move(values));
  }

  template <typename Fixture, int id, typename... Types>
  void AddTypeDimension(const void* instantiation_key, std::tuple<Types...>*) {
    auto& d = fixtures_[kKey<Fixture>]
                  .per_instantiation_[instantiation_key]
                  .types_[id];
    if (!d.names.empty()) return;
    (d.names.push_back(testing::internal::GetTypeName<Types>()), ...);
    SanitizeNames(d.names);
  }

  template <typename Fixture, typename T>
  const T& GetValue(size_t index, int id, const void*,
                    const void* instantiation_key) {
    for (const auto& [i, dim] : fixtures_[kKey<Fixture>]
                                    .per_instantiation_[instantiation_key]
                                    .values_) {
      if (i == id) {
        auto& v = *static_cast<const std::deque<T>*>(dim.values);
        return v[index % v.size()];
      } else {
        index /= dim.names.size();
      }
    }

    Unreachable();
  }

  void RegisterAll() const;

 private:
  PartialTestRegistry() = default;

  template <typename Fixture>
  static constexpr const void* kKey = &kKey<Fixture>;

  template <typename T>
  static std::string MakeName(const T& value) {
    if constexpr (google::protobuf::is_proto_enum<T>::value) {
      auto* e = google::protobuf::GetEnumDescriptor<T>()->FindValueByNumber(value);
      return e != nullptr ? std::string(e->name())
                          : absl::StrCat(static_cast<int>(value));
    } else {
      return absl::StrCat(value);
    }
  }

  struct PerFixture {
    const char* test_suite_name;
    const char* test_name;
    const char* file;
    int line;

    struct PerInstantiation {
      absl::AnyInvocable<void(size_t index) const&> do_register;
      struct TypeDimension {
        std::vector<std::string> names;
      };
      struct ValueDimension {
        std::vector<std::string> names;
        void* values;
      };
      std::string TypeName(absl::Span<const size_t> v) const;
      std::string ValueName(size_t i) const;

      std::vector<int> instantiation_key;

      // We want these sorted.
      absl::btree_map<int, TypeDimension> types_;
      absl::btree_map<int, ValueDimension> values_;
    };
    absl::flat_hash_map<const void*, PerInstantiation> per_instantiation_;
  };
  // We need pointer stability.
  absl::flat_hash_map<const void*, PerFixture> fixtures_;
};

template <typename T, size_t N>
auto ChooseValueImpl(const T (&values)[N]) {
  // We use std::deque because std::vector<bool> is problematic
  return std::deque<T>(std::begin(values), std::end(values));
}

template <typename F>
inline std::false_type Run = (F{}(), std::false_type{});

template <typename Fixture, int id, size_t... instantiation_types, typename F>
const auto& TestChooseValueSelector(size_t index, F) {
  static constexpr auto reg = [] {
    PartialTestRegistry::Get().AddValueDimension<Fixture>(
        id, PartialTestRegistry::kVectorKey<instantiation_types...>, F{}());
  };
  return PartialTestRegistry::Get()
      .GetValue<Fixture, typename decltype(F{}())::value_type>(
          index, id, &Run<decltype(reg)>,
          PartialTestRegistry::kVectorKey<instantiation_types...>);
}

template <int id, size_t... instantiation_types>
constexpr int GetTypeIndexForId() {
  if constexpr (sizeof...(instantiation_types) != 0) {
    auto input = std::array{instantiation_types...};
    for (size_t i = 0; i < input.size(); i += 2) {
      if (input[i] == id) return input[i + 1];
    }
  }
  return 0;
}

template <typename... T>
using TestChooseTypeParams = std::tuple<T...>;

template <typename T, auto*>
using WithRegistration = T;

template <typename Fixture, size_t id, size_t i, size_t size,
          size_t... instantiation_types>
void InstantiateAllTypes() {
  if constexpr (sizeof...(instantiation_types) != 0 &&
                id >= std::array{instantiation_types..., size_t{}}[0]) {
    // We instantiate in only one direction.
  } else if constexpr (i == size) {
    return;
  } else {
    if constexpr (i != 0) {
      Fixture::template Instantiate_<id, i, instantiation_types...>();
    }
    InstantiateAllTypes<Fixture, id, i + 1, size, instantiation_types...>();
  }
}

template <typename Fixture, typename Types, int id,
          size_t... instantiation_types>
struct TestChooseTypeSelector {
  static constexpr auto Register = []() {
    PartialTestRegistry::Get().AddTypeDimension<Fixture, id>(
        PartialTestRegistry::kVectorKey<instantiation_types...>,
        static_cast<Types*>(nullptr));
    InstantiateAllTypes<Fixture, id, 0, std::tuple_size_v<Types>,
                        instantiation_types...>();
    PartialTestRegistry::Get().AddDoRegister<Fixture, instantiation_types...>();
  };
  using type = WithRegistration<
      std::tuple_element_t<GetTypeIndexForId<id, instantiation_types...>(),
                           Types>,
      &Run<decltype(Register)>>;
};

}  // namespace protobuf
}  // namespace google::internal

#endif  // GOOGLE_PROTOBUF_PROTOBUF_TEST_H__
