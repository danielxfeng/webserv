#include <gtest/gtest.h>
#include "../../includes/TinyJson.hpp"

namespace {

template <typename T>
const T& as(const JsonValue& v) {
  return std::get<T>(v);
}

template <typename T>
bool is(const JsonValue& v) {
  return std::holds_alternative<T>(v);
}

} // namespace


TEST(TestTinyJson, TestTest) {
  EXPECT_EQ(1, 1);
}


TEST(TinyJson, EmptyStringThrows) {
  EXPECT_THROW(TinyJson::parse(""), std::runtime_error);
}

// 2) parse a string with prefix different white spaces
TEST(TinyJson, StringWithPrefixWhitespace) {
  auto v = TinyJson::parse("   \t \n \"hello\""); // leading spaces/tabs/newline
  ASSERT_TRUE(is<std::string>(v));
  EXPECT_EQ(as<std::string>(v), "hello");
}

// 3) parse a string with postfix (extra trailing characters should be rejected)
TEST(TinyJson, StringWithPostfixGarbageThrows) {
  EXPECT_THROW(TinyJson::parse("\"hello\" trailing"), std::runtime_error);
}

// 4) parse a string with pre/postfix different white spaces
TEST(TinyJson, StringWithSurroundingWhitespaceOK) {
  auto v = TinyJson::parse("\r \n  \"hi\"   \t \n");
  ASSERT_TRUE(is<std::string>(v));
  EXPECT_EQ(as<std::string>(v), "hi");
}

// 5) parse a json with primitive types
TEST(TinyJson, Primitives) {
  // true
  {
    auto v = TinyJson::parse("true");
    ASSERT_TRUE(is<bool>(v));
    EXPECT_TRUE(as<bool>(v));
  }
  // false
  {
    auto v = TinyJson::parse("false");
    ASSERT_TRUE(is<bool>(v));
    EXPECT_FALSE(as<bool>(v));
  }
  // null
  {
    auto v = TinyJson::parse("null");
    ASSERT_TRUE(is<std::nullptr_t>(v));
  }
  // number: integer-like
  {
    auto v = TinyJson::parse("123");
    ASSERT_TRUE(is<double>(v));
    EXPECT_DOUBLE_EQ(as<double>(v), 123.0);
  }
  // number: negative zero allowed
  {
    auto v = TinyJson::parse("-0");
    ASSERT_TRUE(is<double>(v));
    EXPECT_DOUBLE_EQ(as<double>(v), -0.0);
  }
  // number: floating
  {
    auto v = TinyJson::parse("3.1415");
    ASSERT_TRUE(is<double>(v));
    EXPECT_DOUBLE_EQ(as<double>(v), 3.1415);
  }
  // number: leading zeros not allowed (e.g., 0123)
  {
    EXPECT_THROW(TinyJson::parse("0123"), std::runtime_error);
  }
}

// 6) parse a json with array
TEST(TinyJson, Arrays) {
  {
    auto v = TinyJson::parse("[1, 2, 3]");
    ASSERT_TRUE(is<JsonArray>(v));
    const auto& arr = as<JsonArray>(v);
    ASSERT_EQ(arr.size(), 3u);
    EXPECT_DOUBLE_EQ(as<double>(arr[0]), 1.0);
    EXPECT_DOUBLE_EQ(as<double>(arr[1]), 2.0);
    EXPECT_DOUBLE_EQ(as<double>(arr[2]), 3.0);
  }

  {
    auto v = TinyJson::parse("[ true , null , \"x\" , [0,1] ]");
    ASSERT_TRUE(is<JsonArray>(v));
    const auto& arr = as<JsonArray>(v);
    ASSERT_EQ(arr.size(), 4u);

    EXPECT_TRUE(is<bool>(arr[0]) && as<bool>(arr[0]) == true);
    EXPECT_TRUE(is<std::nullptr_t>(arr[1]));
    EXPECT_TRUE(is<std::string>(arr[2]) && as<std::string>(arr[2]) == "x");

    ASSERT_TRUE(is<JsonArray>(arr[3]));
    const auto& inner = as<JsonArray>(arr[3]);
    ASSERT_EQ(inner.size(), 2u);
    EXPECT_DOUBLE_EQ(as<double>(inner[0]), 0.0);
    EXPECT_DOUBLE_EQ(as<double>(inner[1]), 1.0);
  }

  // malformed (trailing comma)
  EXPECT_THROW(TinyJson::parse("[1,]"), std::runtime_error);
}

// 7) parse a json with object
TEST(TinyJson, Objects) {
  auto v = TinyJson::parse("{\"k\":1, \"b\":true, \"s\":\"hi\", \"n\":null, \"arr\":[1,2], \"o\":{}}");
  ASSERT_TRUE(is<JsonObject>(v));
  const auto& obj = as<JsonObject>(v);

  ASSERT_EQ(obj.count("k"), 1u);
  ASSERT_EQ(obj.count("b"), 1u);
  ASSERT_EQ(obj.count("s"), 1u);
  ASSERT_EQ(obj.count("n"), 1u);
  ASSERT_EQ(obj.count("arr"), 1u);
  ASSERT_EQ(obj.count("o"), 1u);

  EXPECT_DOUBLE_EQ(as<double>(obj.at("k")), 1.0);
  EXPECT_TRUE(as<bool>(obj.at("b")));
  EXPECT_EQ(as<std::string>(obj.at("s")), "hi");
  EXPECT_TRUE(is<std::nullptr_t>(obj.at("n")));

  const auto& arr = as<JsonArray>(obj.at("arr"));
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_DOUBLE_EQ(as<double>(arr[0]), 1.0);
  EXPECT_DOUBLE_EQ(as<double>(arr[1]), 2.0);

  const auto& emptyObj = as<JsonObject>(obj.at("o"));
  EXPECT_TRUE(emptyObj.empty());
}

// Extra: ensure surrounding whitespace for objects is fine
TEST(TinyJson, ObjectWithWhitespace) {
  auto v = TinyJson::parse(" \n { \"a\" :  10 }  \t ");
  ASSERT_TRUE(is<JsonObject>(v));
  const auto& obj = as<JsonObject>(v);
  EXPECT_DOUBLE_EQ(as<double>(obj.at("a")), 10.0);
}

// Extra: postfix garbage after valid object should throw
TEST(TinyJson, ObjectWithPostfixGarbageThrows) {
  EXPECT_THROW(TinyJson::parse("{\"a\":1} trailing"), std::runtime_error);
}

// (Optional) A couple of TinyJson::as<T> tests if your template supports numeric casting
TEST(TinyJsonAs, NumericCasts) {
  JsonValue dv = 42.0;
  EXPECT_NO_THROW({
    int x = TinyJson::as<int>(dv); // should succeed for 42.0
    EXPECT_EQ(x, 42);
  });

  JsonValue fv = 1.5;
  // depending on your implementation, non-integer double->int may throw
  EXPECT_ANY_THROW({
    (void)TinyJson::as<int>(fv);
  });
}