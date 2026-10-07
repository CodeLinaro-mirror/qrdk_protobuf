// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/protobuf_test.h"

#include <deque>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/unittest.pb.h"

namespace google::protobuf::internal {
namespace {

using ::proto2_unittest::FOREIGN_BAR;
using ::proto2_unittest::FOREIGN_BAZ;
using ::proto2_unittest::FOREIGN_FOO;
using ::proto2_unittest::ForeignEnum;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::StartsWith;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

class ProtobufTestFixture : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    EXPECT_EQ(set_up_suite_count_, 0);
    EXPECT_EQ(set_up_count_, 0);
    ++set_up_suite_count_;
  }

  static void TearDownTestSuite() {
    EXPECT_EQ(set_up_suite_count_, 1);
    EXPECT_EQ(tear_down_suite_count_, 0);
    EXPECT_EQ(regular_test_f_runs_, 1);
    EXPECT_EQ(no_param_runs_, 1);
    EXPECT_EQ(set_up_count_, 1 + 1 + 3 + 6 + 3 + 4);
    EXPECT_EQ(tear_down_count_, 1 + 1 + 3 + 6 + 3 + 4);
    EXPECT_THAT(single_dim_values_, ElementsAre(10, 20, 30));
    EXPECT_THAT(multi_dim_values_,
                ElementsAre(Pair(1, "foo"), Pair(2, "foo"), Pair(1, "bar"),
                            Pair(2, "bar"), Pair(1, "baz"), Pair(2, "baz")));
    EXPECT_THAT(single_type_names_, ElementsAre("int", "double", "bool"));
    EXPECT_THAT(type_and_value_runs_,
                ElementsAre(Pair(10, "int"), Pair(20, "int"),
                            Pair(10, "double"), Pair(20, "double")));
    ++tear_down_suite_count_;
  }

  static inline int set_up_suite_count_ = 0;
  static inline int tear_down_suite_count_ = 0;
  static inline int set_up_count_ = 0;
  static inline int tear_down_count_ = 0;
  static inline int regular_test_f_runs_ = 0;
  static inline int no_param_runs_ = 0;
  static inline std::vector<int> single_dim_values_;
  static inline std::vector<std::pair<int, std::string>> multi_dim_values_;
  static inline std::vector<std::string> single_type_names_;
  static inline std::vector<std::pair<int, std::string>> type_and_value_runs_;

 protected:
  void SetUp() override {
    EXPECT_EQ(set_up_suite_count_, 1);
    EXPECT_EQ(tear_down_suite_count_, 0);
    fixture_state_ = 42;
    ++set_up_count_;
  }

  void TearDown() override { ++tear_down_count_; }

  int fixture_state_ = 0;
};

TEST_F(ProtobufTestFixture, RegularTestF) {
  EXPECT_EQ(fixture_state_, 42);
  ++regular_test_f_runs_;
}

PB_TEST_F(ProtobufTestFixture, NoParameters) {
  EXPECT_EQ(fixture_state_, 42);
  ++no_param_runs_;
  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_STREQ(test_info->name(), "NoParameters");
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, SingleDimension) {
  EXPECT_EQ(fixture_state_, 42);
  int v = PB_TEST_CHOOSE_VALUE(10, 20, 30);
  single_dim_values_.push_back(v);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleDimension/", v));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, MultiDimension) {
  EXPECT_EQ(fixture_state_, 42);
  int a = PB_TEST_CHOOSE_VALUE(1, 2);
  absl::string_view b = PB_TEST_CHOOSE_VALUE("foo", "bar", "baz");
  multi_dim_values_.emplace_back(a, std::string(b));

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("MultiDimension/", a, "/", b));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(a, "/", b)));
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, SingleType) {
  EXPECT_EQ(fixture_state_, 42);
  using T = PB_TEST_CHOOSE_TYPE(int, double, bool);
  std::string type_name = ::testing::internal::GetTypeName<T>();
  single_type_names_.push_back(type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleType/", type_name));
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
}

PB_TEST_F(ProtobufTestFixture, TypeAndValue) {
  EXPECT_EQ(fixture_state_, 42);
  int v = PB_TEST_CHOOSE_VALUE(10, 20);
  using T = PB_TEST_CHOOSE_TYPE(int, double);
  std::string type_name = ::testing::internal::GetTypeName<T>();
  type_and_value_runs_.emplace_back(v, type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(),
            absl::StrCat("TypeAndValue/", v, "/", type_name));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
}

std::vector<bool>& RecordedBools() {
  static auto* v = new std::vector<bool>();
  return *v;
}

std::vector<ForeignEnum>& RecordedEnums() {
  static auto* v = new std::vector<ForeignEnum>();
  return *v;
}

std::vector<std::string>& RecordedSanitizedInputs() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedDupeInputs() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::pair<int, int>>& RecordedConditionalInputs() {
  static auto* v = new std::vector<std::pair<int, int>>();
  return *v;
}

std::vector<std::string>& RecordedConstRefStrings() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<int>& RecordedSameLineSums() {
  static auto* v = new std::vector<int>();
  return *v;
}

std::vector<std::string>& RecordedMultiTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypesAndValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypeDependentValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypeDependentTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedDupeTypeTestNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedSanitizedTypeTestNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedSameLineTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

int g_macro_first_runs = 0;
int g_macro_second_runs = 0;

PB_TEST(ProtobufStandaloneTest, ThreeDimensions) {
  int x = PB_TEST_CHOOSE_VALUE(1, 2);
  absl::string_view y = PB_TEST_CHOOSE_VALUE("a", "b");
  int z = PB_TEST_CHOOSE_VALUE(100, 200);
  EXPECT_TRUE(x == 1 || x == 2);
  EXPECT_TRUE(y == "a" || y == "b");
  EXPECT_TRUE(z == 100 || z == 200);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("ThreeDimensions/", x, "/", y, "/", z));
}

PB_TEST(ProtobufStandaloneTest, BoolValues) {
  const bool& b = PB_TEST_CHOOSE_VALUE(false, true);
  RecordedBools().push_back(b);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("BoolValues/", b));
}

PB_TEST(ProtobufStandaloneTest, ProtoEnumValues) {
  ForeignEnum e = PB_TEST_CHOOSE_VALUE(FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ,
                                       static_cast<ForeignEnum>(999),
                                       static_cast<ForeignEnum>(-1));
  RecordedEnums().push_back(e);
}

PB_TEST(ProtobufStandaloneTest, SanitizesNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE("", "///", "__a//b--", "foo-bar", "hello world!");
  int n = PB_TEST_CHOOSE_VALUE(-5, 5, 10);
  RecordedSanitizedInputs().push_back(absl::StrCat(s, ":", n));
}

PB_TEST(ProtobufStandaloneTest, DisambiguatesDuplicateNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE("", "_", "a_b", "a-b", "a/b", "a b");
  RecordedDupeInputs().emplace_back(s);
}

PB_TEST(ProtobufStandaloneTest, ConditionalChooseValue) {
  int a = PB_TEST_CHOOSE_VALUE(1, 2);
  int b = 0;
  if (a == 1) {
    b = PB_TEST_CHOOSE_VALUE(10, 20);
  }
  RecordedConditionalInputs().emplace_back(a, b);
}

PB_TEST(ProtobufStandaloneTest, ReturnsConstReference) {
  decltype(auto) s =
      PB_TEST_CHOOSE_VALUE(std::string("hello"), std::string("world"));
  EXPECT_TRUE((std::is_same_v<decltype(s), const std::string&>));
  absl::string_view sv = s;
  RecordedConstRefStrings().emplace_back(sv);
}

PB_TEST(ProtobufStandaloneTest, SameLineChooseValue) {
  int v = PB_TEST_CHOOSE_VALUE(1, 2) + PB_TEST_CHOOSE_VALUE(10, 20);
  RecordedSameLineSums().push_back(v);
}

PB_TEST(ProtobufStandaloneTest, MultipleTypeDimensions) {
  using T1 = PB_TEST_CHOOSE_TYPE(int, double, float);
  using T2 = PB_TEST_CHOOSE_TYPE(char, bool);
  std::string type_pair =
      absl::StrCat(::testing::internal::GetTypeName<T1>(), "/",
                   ::testing::internal::GetTypeName<T2>());
  RecordedMultiTypes().push_back(type_pair);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_EQ(test_info->name(),
            absl::StrCat("MultipleTypeDimensions/", type_pair));
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_THAT(test_info->type_param(), StrEq(type_pair));
}

PB_TEST(ProtobufStandaloneTest, TypesAndValuesCombined) {
  int a = PB_TEST_CHOOSE_VALUE(1, 2);
  absl::string_view b = PB_TEST_CHOOSE_VALUE("x", "y");
  using T1 = PB_TEST_CHOOSE_TYPE(int, double);
  using T2 = PB_TEST_CHOOSE_TYPE(char, bool);
  std::string value_part = absl::StrCat(a, "/", b);
  std::string type_part =
      absl::StrCat(::testing::internal::GetTypeName<T1>(), "/",
                   ::testing::internal::GetTypeName<T2>());
  RecordedTypesAndValues().push_back(absl::StrCat(value_part, "/", type_part));

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_EQ(test_info->name(), absl::StrCat("TypesAndValuesCombined/",
                                            value_part, "/", type_part));
  EXPECT_THAT(test_info->value_param(), StrEq(value_part));
  EXPECT_THAT(test_info->type_param(), StrEq(type_part));
}

PB_TEST(ProtobufStandaloneTest, TypeDependentValues) {
  using T = PB_TEST_CHOOSE_TYPE(int, double);
  T value = PB_TEST_CHOOSE_VALUE(T{0}, T{1});
  RecordedTypeDependentValues().push_back(
      absl::StrCat(::testing::internal::GetTypeName<T>(), ":", value));
}

PB_TEST(ProtobufStandaloneTest, TypeDependentTypes) {
  using T = PB_TEST_CHOOSE_TYPE(int, double);
  using U = PB_TEST_CHOOSE_TYPE(std::vector<T>, std::deque<T>);
  EXPECT_TRUE((std::is_same_v<typename U::value_type, T>));
  U container = {T{1}, T{2}};
  EXPECT_EQ(container.size(), 2);
  absl::string_view container_type =
      std::is_same_v<U, std::vector<T>> ? "vector" : "deque";
  RecordedTypeDependentTypes().push_back(
      absl::StrCat(::testing::internal::GetTypeName<T>(), ":", container_type));
}

PB_TEST(ProtobufStandaloneTest, DisambiguatesDuplicateTypes) {
  using IntAlias = int;
  using T = PB_TEST_CHOOSE_TYPE(int, IntAlias, int);
  EXPECT_TRUE((std::is_same_v<T, int>));
  RecordedDupeTypeTestNames().emplace_back(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

PB_TEST(ProtobufStandaloneTest, SanitizesTypeNames) {
  using T = PB_TEST_CHOOSE_TYPE(int, unsigned int, int*, ForeignEnum);
  EXPECT_GT(sizeof(T), 0);
  RecordedSanitizedTypeTestNames().emplace_back(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

PB_TEST(ProtobufStandaloneTest, SameLineChooseType) {
  using PairType = std::pair<PB_TEST_CHOOSE_TYPE(int, double),
                             PB_TEST_CHOOSE_TYPE(char, bool)>;
  RecordedSameLineTypes().push_back(absl::StrCat(
      ::testing::internal::GetTypeName<typename PairType::first_type>(), "/",
      ::testing::internal::GetTypeName<typename PairType::second_type>()));
}

#define PB_TEST_DEFINE_TWO_ON_SAME_LINE()                \
  PB_TEST(ProtobufStandaloneTest, MacroExpandedFirst) {  \
    ++g_macro_first_runs;                                \
  }                                                      \
  PB_TEST(ProtobufStandaloneTest, MacroExpandedSecond) { \
    ++g_macro_second_runs;                               \
  }
PB_TEST_DEFINE_TWO_ON_SAME_LINE()
#undef PB_TEST_DEFINE_TWO_ON_SAME_LINE

PB_TEST(ProtobufStandaloneTest, RepeatedExecutionReturnsSameValue) {
  auto choose = [&] { return PB_TEST_CHOOSE_VALUE(1, 2); };
  int first = choose();
  int second = choose();
  EXPECT_EQ(first, second);
}

class VerificationEnvironment : public ::testing::Environment {
 public:
  void TearDown() override {
    EXPECT_EQ(ProtobufTestFixture::set_up_suite_count_, 1);
    EXPECT_EQ(ProtobufTestFixture::tear_down_suite_count_, 1);
    EXPECT_THAT(RecordedBools(), ElementsAre(false, true));
    EXPECT_THAT(RecordedEnums(),
                ElementsAre(FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ,
                            static_cast<ForeignEnum>(999),
                            static_cast<ForeignEnum>(-1)));
    EXPECT_THAT(RecordedSanitizedInputs(),
                ElementsAre(":-5", "///:-5", "__a//b--:-5", "foo-bar:-5",
                            "hello world!:-5", ":5", "///:5", "__a//b--:5",
                            "foo-bar:5", "hello world!:5", ":10", "///:10",
                            "__a//b--:10", "foo-bar:10", "hello world!:10"));
    EXPECT_THAT(RecordedDupeInputs(),
                ElementsAre("", "_", "a_b", "a-b", "a/b", "a b"));
    EXPECT_THAT(RecordedConditionalInputs(),
                ElementsAre(Pair(1, 10), Pair(2, 0), Pair(1, 20), Pair(2, 0)));
    EXPECT_THAT(RecordedConstRefStrings(), ElementsAre("hello", "world"));
    EXPECT_THAT(RecordedSameLineSums(), ElementsAre(11, 12, 21, 22));
    EXPECT_THAT(RecordedMultiTypes(),
                ElementsAre("int/char", "double/char", "double/bool",
                            "float/char", "float/bool", "int/bool"));
    EXPECT_THAT(
        RecordedTypesAndValues(),
        ElementsAre("1/x/int/char", "2/x/int/char", "1/y/int/char",
                    "2/y/int/char", "1/x/double/char", "2/x/double/char",
                    "1/y/double/char", "2/y/double/char", "1/x/double/bool",
                    "2/x/double/bool", "1/y/double/bool", "2/y/double/bool",
                    "1/x/int/bool", "2/x/int/bool", "1/y/int/bool",
                    "2/y/int/bool"));
    EXPECT_THAT(RecordedTypeDependentValues(),
                ElementsAre("int:0", "int:1", "double:0", "double:1"));
    EXPECT_THAT(RecordedTypeDependentTypes(),
                ElementsAre("int:vector", "double:vector", "double:deque",
                            "int:deque"));
    EXPECT_THAT(RecordedDupeTypeTestNames(),
                ElementsAre("DisambiguatesDuplicateTypes/int",
                            "DisambiguatesDuplicateTypes/int_2",
                            "DisambiguatesDuplicateTypes/int_3"));
    EXPECT_THAT(
        RecordedSanitizedTypeTestNames(),
        ElementsAre("SanitizesTypeNames/int", "SanitizesTypeNames/unsigned_int",
                    "SanitizesTypeNames/intPtr",
                    "SanitizesTypeNames/proto2_unittest_ForeignEnum"));
    EXPECT_THAT(
        RecordedSameLineTypes(),
        ElementsAre("int/char", "double/char", "double/bool", "int/bool"));
    EXPECT_EQ(g_macro_first_runs, 1);
    EXPECT_EQ(g_macro_second_runs, 1);
  }
};

[[maybe_unused]] const auto* const kVerificationEnv =
    ::testing::AddGlobalTestEnvironment(new VerificationEnvironment());

TEST(ProtobufTestRegistryTest, RegistersExpectedTestsWithoutDummySuite) {
  const auto* unit_test = ::testing::UnitTest::GetInstance();
  std::vector<std::string> suite_names;
  const ::testing::TestSuite* standalone_suite = nullptr;

  for (int i = 0; i < unit_test->total_test_suite_count(); ++i) {
    const auto* suite = unit_test->GetTestSuite(i);
    suite_names.emplace_back(suite->name());
    if (absl::string_view(suite->name()) == "ProtobufStandaloneTest") {
      standalone_suite = suite;
    }
  }

  EXPECT_THAT(suite_names, UnorderedElementsAre("ProtobufTestFixture",
                                                "ProtobufStandaloneTest",
                                                "ProtobufTestRegistryTest"));

  ASSERT_NE(standalone_suite, nullptr);
  std::vector<std::string> test_names;
  for (int i = 0; i < standalone_suite->total_test_count(); ++i) {
    test_names.emplace_back(standalone_suite->GetTestInfo(i)->name());
  }
  EXPECT_THAT(
      test_names,
      ElementsAre(
          "ThreeDimensions/1/a/100", "ThreeDimensions/2/a/100",
          "ThreeDimensions/1/b/100", "ThreeDimensions/2/b/100",
          "ThreeDimensions/1/a/200", "ThreeDimensions/2/a/200",
          "ThreeDimensions/1/b/200", "ThreeDimensions/2/b/200", "BoolValues/0",
          "BoolValues/1", "ProtoEnumValues/FOREIGN_FOO",
          "ProtoEnumValues/FOREIGN_BAR", "ProtoEnumValues/FOREIGN_BAZ",
          "ProtoEnumValues/999", "ProtoEnumValues/1", "SanitizesNames/_/5",
          "SanitizesNames/_2/5", "SanitizesNames/a_b/5",
          "SanitizesNames/foo_bar/5", "SanitizesNames/hello_world/5",
          "SanitizesNames/_/5_2", "SanitizesNames/_2/5_2",
          "SanitizesNames/a_b/5_2", "SanitizesNames/foo_bar/5_2",
          "SanitizesNames/hello_world/5_2", "SanitizesNames/_/10",
          "SanitizesNames/_2/10", "SanitizesNames/a_b/10",
          "SanitizesNames/foo_bar/10", "SanitizesNames/hello_world/10",
          "DisambiguatesDuplicateNames/_", "DisambiguatesDuplicateNames/_2",
          "DisambiguatesDuplicateNames/a_b",
          "DisambiguatesDuplicateNames/a_b_2",
          "DisambiguatesDuplicateNames/a_b_3",
          "DisambiguatesDuplicateNames/a_b_4", "ConditionalChooseValue/1/10",
          "ConditionalChooseValue/2/10", "ConditionalChooseValue/1/20",
          "ConditionalChooseValue/2/20", "ReturnsConstReference/hello",
          "ReturnsConstReference/world", "SameLineChooseValue/1/10",
          "SameLineChooseValue/2/10", "SameLineChooseValue/1/20",
          "SameLineChooseValue/2/20", "MultipleTypeDimensions/int/char",
          "MultipleTypeDimensions/double/char",
          "MultipleTypeDimensions/double/bool",
          "MultipleTypeDimensions/float/char",
          "MultipleTypeDimensions/float/bool",
          "MultipleTypeDimensions/int/bool",
          "TypesAndValuesCombined/1/x/int/char",
          "TypesAndValuesCombined/2/x/int/char",
          "TypesAndValuesCombined/1/y/int/char",
          "TypesAndValuesCombined/2/y/int/char",
          "TypesAndValuesCombined/1/x/double/char",
          "TypesAndValuesCombined/2/x/double/char",
          "TypesAndValuesCombined/1/y/double/char",
          "TypesAndValuesCombined/2/y/double/char",
          "TypesAndValuesCombined/1/x/double/bool",
          "TypesAndValuesCombined/2/x/double/bool",
          "TypesAndValuesCombined/1/y/double/bool",
          "TypesAndValuesCombined/2/y/double/bool",
          "TypesAndValuesCombined/1/x/int/bool",
          "TypesAndValuesCombined/2/x/int/bool",
          "TypesAndValuesCombined/1/y/int/bool",
          "TypesAndValuesCombined/2/y/int/bool", "TypeDependentValues/0/int",
          "TypeDependentValues/1/int", "TypeDependentValues/0/double",
          "TypeDependentValues/1/double",
          StartsWith("TypeDependentTypes/int/std_vector_int_"),
          StartsWith("TypeDependentTypes/double/std_vector_double_"),
          StartsWith("TypeDependentTypes/double/std_deque_double_"),
          StartsWith("TypeDependentTypes/int/std_deque_int_"),
          "DisambiguatesDuplicateTypes/int",
          "DisambiguatesDuplicateTypes/int_2",
          "DisambiguatesDuplicateTypes/int_3", "SanitizesTypeNames/int",
          "SanitizesTypeNames/unsigned_int", "SanitizesTypeNames/intPtr",
          "SanitizesTypeNames/proto2_unittest_ForeignEnum",
          "SameLineChooseType/int/char", "SameLineChooseType/double/char",
          "SameLineChooseType/double/bool", "SameLineChooseType/int/bool",
          "MacroExpandedFirst", "MacroExpandedSecond",
          "RepeatedExecutionReturnsSameValue/1",
          "RepeatedExecutionReturnsSameValue/2"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google::internal
