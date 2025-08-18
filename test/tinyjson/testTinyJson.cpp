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

template <typename T>
const T& as_v(const JsonValue& v) { return std::get<T>(v); }

} // namespace


TEST(TinyJson, TestTest) {
  EXPECT_EQ(1, 1);
}


TEST(TinyJson, EmptyStringThrows) {
  EXPECT_THROW(TinyJson::parse(""), std::runtime_error);
}

TEST(TinyJson, WhitespaceOK) {
  auto v = TinyJson::parse(" \n { \"a\" :  10 }  \t ");
  ASSERT_TRUE(is<JsonObject>(v));
  EXPECT_DOUBLE_EQ(std::get<double>(as<JsonObject>(v).at("a")), 10.0);
}

TEST(TinyJson, PostfixGarbageRejected) {
  EXPECT_THROW(TinyJson::parse("\"a\" trailing"), std::runtime_error);
  EXPECT_THROW(TinyJson::parse("true]"), std::runtime_error);
  EXPECT_THROW(TinyJson::parse("{\"a\":1} {}"), std::runtime_error);
}

// parse a string with prefix different white spaces
TEST(TinyJson, StringWithPrefixWhitespace) {
  auto v = TinyJson::parse("   \t \n \"hello\""); // leading spaces/tabs/newline
  ASSERT_TRUE(is<std::string>(v));
  EXPECT_EQ(as<std::string>(v), "hello");
}

// parse a string with postfix (extra trailing characters should be rejected)
TEST(TinyJson, StringWithPostfixGarbageThrows) {
  EXPECT_THROW(TinyJson::parse("\"hello\" trailing"), std::runtime_error);
}

// parse a string with pre/postfix different white spaces
TEST(TinyJson, StringWithSurroundingWhitespaceOK) {
  auto v = TinyJson::parse("\r \n  \"hi\"   \t \n");
  ASSERT_TRUE(is<std::string>(v));
  EXPECT_EQ(as<std::string>(v), "hi");
}

// parse a json with primitive types
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

TEST(TinyJson, ValidBasicAndEscapes) {
  // Simple
  {
    auto v = TinyJson::parse("\"hello\"");
    ASSERT_TRUE(is<std::string>(v));
    EXPECT_EQ(as<std::string>(v), "hello");
  }
  // Escaped quotes, slash and backslash
  {
    auto v = TinyJson::parse("\"\\\"\\\\\\/\"");
    ASSERT_TRUE(is<std::string>(v));
    EXPECT_EQ(as<std::string>(v), "\"\\/");
  }
  // Control escape sequences
  {
    auto v = TinyJson::parse("\"line\\nbreak\\tand\\rcarriage\\fform\\bback\"");
    ASSERT_TRUE(is<std::string>(v));
    EXPECT_EQ(as<std::string>(v), "line\nbreak\tand\rcarriage\fform\bback");
  }
}

TEST(TinyJson, InvalidStrings) {
  // Unescaped control character (< 0x20)
  EXPECT_THROW(TinyJson::parse("\"\x01\""), std::runtime_error);
  // Invalid escape sequence
  EXPECT_THROW(TinyJson::parse("\"\\x\""), std::runtime_error);
  // Unterminated string
  EXPECT_THROW(TinyJson::parse("\"unterminated"), std::runtime_error);
  // Unescaped quote inside
  EXPECT_THROW(TinyJson::parse("\"bad \" quote\""), std::runtime_error);
  // Unicode escape not implemented
  EXPECT_THROW(TinyJson::parse("\"\\u0041\""), std::runtime_error);
}


TEST(TinyJson, TrueFalseNullCaseAndWhitespace) {
  {
    auto v = TinyJson::parse("  true  ");
    ASSERT_TRUE(is<bool>(v));
    EXPECT_TRUE(as<bool>(v));
  }
  {
    auto v = TinyJson::parse("\tfalse\n");
    ASSERT_TRUE(is<bool>(v));
    EXPECT_FALSE(as<bool>(v));
  }
  {
    auto v = TinyJson::parse("\r null ");
    ASSERT_TRUE(is<std::nullptr_t>(v));
  }

  // Case must be lowercase in JSON
  EXPECT_THROW(TinyJson::parse("True"), std::runtime_error);
  EXPECT_THROW(TinyJson::parse("False"), std::runtime_error);
  EXPECT_THROW(TinyJson::parse("NULL"), std::runtime_error);
}

TEST(TinyJson, InvalidNumbersExtended) {
  // --- Leading zeros ---
  EXPECT_THROW(TinyJson::parse("0123"), std::runtime_error);   // plain integer with leading 0
  EXPECT_THROW(TinyJson::parse("00"), std::runtime_error);     // double zero
  EXPECT_THROW(TinyJson::parse("-0123"), std::runtime_error);  // negative with leading zero
  EXPECT_THROW(TinyJson::parse("01.23"), std::runtime_error);  // float with leading zero

  // --- Garbage suffixes ---
  EXPECT_THROW(TinyJson::parse("1a"), std::runtime_error);       // letter suffix
  EXPECT_THROW(TinyJson::parse("22.44b"), std::runtime_error);   // float + garbage
  EXPECT_THROW(TinyJson::parse("-3.5xyz"), std::runtime_error);  // multiple letters

  // --- Invalid signs ---
  EXPECT_THROW(TinyJson::parse("+1"), std::runtime_error);    // '+' not allowed by JSON
  EXPECT_THROW(TinyJson::parse("+-2"), std::runtime_error);   // double sign
  EXPECT_THROW(TinyJson::parse("--5"), std::runtime_error);   // double minus

  // --- Decimal point misuse ---
  EXPECT_THROW(TinyJson::parse("."), std::runtime_error);     // lone dot
  EXPECT_THROW(TinyJson::parse("1."), std::runtime_error);    // trailing dot
  EXPECT_THROW(TinyJson::parse(".5"), std::runtime_error);    // missing integer part

  // --- Exponent notation (scientific) ---
  EXPECT_THROW(TinyJson::parse("1e"), std::runtime_error);     // missing exponent digits
  EXPECT_THROW(TinyJson::parse("1e+"), std::runtime_error);    // sign but no number
  EXPECT_THROW(TinyJson::parse("1e-"), std::runtime_error);    // negative sign but no number
  EXPECT_THROW(TinyJson::parse("1e99999999999999999999"), std::out_of_range); // overflow exponent
  EXPECT_THROW(TinyJson::parse("1.2.3"), std::runtime_error);  // multiple dots

  // --- Weird edge tokens ---
  EXPECT_THROW(TinyJson::parse("-i"), std::runtime_error);     // invalid identifier
  EXPECT_THROW(TinyJson::parse("NaN"), std::runtime_error);    // NaN not valid in JSON
  EXPECT_THROW(TinyJson::parse("Infinity"), std::runtime_error); // Infinity not valid in JSON
  EXPECT_THROW(TinyJson::parse("-Infinity"), std::runtime_error); // negative Infinity not valid
}

// parse a json with array
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

TEST(TinyJsonArrays, ValidArrays) {
  // Empty
  {
    auto v = TinyJson::parse("[]");
    ASSERT_TRUE(is<JsonArray>(v));
    EXPECT_TRUE(as<JsonArray>(v).empty());
  }
  // Mixed values + whitespace
  {
    auto v = TinyJson::parse("[ true , null , \"x\" , [0,1] ]");
    ASSERT_TRUE(is<JsonArray>(v));
    const auto& arr = as<JsonArray>(v);
    ASSERT_EQ(arr.size(), 4u);
    EXPECT_TRUE(std::get<bool>(arr[0]));
    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(arr[1]));
    EXPECT_EQ(std::get<std::string>(arr[2]), "x");
    const auto& inner = std::get<JsonArray>(arr[3]);
    ASSERT_EQ(inner.size(), 2u);
    EXPECT_DOUBLE_EQ(std::get<double>(inner[0]), 0.0);
    EXPECT_DOUBLE_EQ(std::get<double>(inner[1]), 1.0);
  }
}

TEST(TinyJsonArrays, InvalidArrays) {
  // Trailing comma
  EXPECT_THROW(TinyJson::parse("[1,]"), std::runtime_error);
  // Missing comma
  EXPECT_THROW(TinyJson::parse("[1 2]"), std::runtime_error);
  // Placeholder comma
  EXPECT_THROW(TinyJson::parse("[,]"), std::runtime_error);
  // Unterminated
  EXPECT_THROW(TinyJson::parse("[1, 2"), std::runtime_error);
  // Extra comma at start
  EXPECT_THROW(TinyJson::parse("[,1]"), std::runtime_error);
}


// parse a json with object
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

TEST(TinyJson, ObjectWithWhitespace) {
  auto v = TinyJson::parse(" \n { \"a\" :  10 }  \t ");
  ASSERT_TRUE(is<JsonObject>(v));
  const auto& obj = as<JsonObject>(v);
  EXPECT_DOUBLE_EQ(as<double>(obj.at("a")), 10.0);
}

TEST(TinyJson, ObjectWithPostfixGarbageThrows) {
  EXPECT_THROW(TinyJson::parse("{\"a\":1} trailing"), std::runtime_error);
}

TEST(TinyJson, ValidObjects) {
  // Empty object
  {
    auto v = TinyJson::parse("{}");
    ASSERT_TRUE(is<JsonObject>(v));
    EXPECT_TRUE(as<JsonObject>(v).empty());
  }
  // Basic object with nesting
  {
    auto v = TinyJson::parse("{\"k\":1, \"b\":true, \"s\":\"hi\", \"n\":null, \"arr\":[1,2], \"o\":{}}");
    ASSERT_TRUE(is<JsonObject>(v));
    const auto& obj = as<JsonObject>(v);
    EXPECT_DOUBLE_EQ(std::get<double>(obj.at("k")), 1.0);
    EXPECT_TRUE(std::get<bool>(obj.at("b")));
    EXPECT_EQ(std::get<std::string>(obj.at("s")), "hi");
    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(obj.at("n")));
    const auto& arr = std::get<JsonArray>(obj.at("arr"));
    ASSERT_EQ(arr.size(), 2u);
    EXPECT_DOUBLE_EQ(std::get<double>(arr[0]), 1.0);
    EXPECT_DOUBLE_EQ(std::get<double>(arr[1]), 2.0);
    EXPECT_TRUE(std::get<JsonObject>(obj.at("o")).empty());
  }
}

TEST(TinyJsonObjects, InvalidObjects) {
  // Key must be string
  EXPECT_THROW(TinyJson::parse("{foo:1}"), std::runtime_error);
  // Missing colon
  EXPECT_THROW(TinyJson::parse("{\"a\" 1}"), std::runtime_error);
  // Missing comma between pairs
  EXPECT_THROW(TinyJson::parse("{\"a\":1 \"b\":2}"), std::runtime_error);
  // Trailing comma
  EXPECT_THROW(TinyJson::parse("{\"a\":1,}"), std::runtime_error);
  // Unterminated
  EXPECT_THROW(TinyJson::parse("{\"a\":1"), std::runtime_error);
}

// A couple of TinyJson::as<T> tests if your template supports numeric casting
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

// ---------- as<T> on exact types (non-numeric) ----------
TEST(TinyJsonAs, ExactAlternatives) {
  {
    JsonValue v = std::string("hi");
    EXPECT_EQ(TinyJson::as<std::string>(v), "hi");
  }
  {
    JsonArray a; a.push_back(1.0); a.push_back(true);
    JsonValue v = a;
    const auto got = TinyJson::as<JsonArray>(v);
    ASSERT_EQ(got.size(), 2u);
    EXPECT_TRUE(is<double>(got[0]));
    EXPECT_TRUE(is<bool>(got[1]));
  }
  {
    JsonObject o; o.emplace("k", 1.0);
    JsonValue v = o;
    const auto got = TinyJson::as<JsonObject>(v);
    EXPECT_DOUBLE_EQ(as_v<double>(got.at("k")), 1.0);
  }
  {
    JsonValue v = nullptr;
    EXPECT_TRUE(is<std::nullptr_t>(TinyJson::as<std::nullptr_t>(v)));
  }
}

// ---------- as<bool> semantics ----------
TEST(TinyJsonAs, BoolBehavior) {
  {
    JsonValue v = true;
    EXPECT_TRUE(TinyJson::as<bool>(v));
  }
  {
    JsonValue v = false;
    EXPECT_FALSE(TinyJson::as<bool>(v));
  }
  {
    JsonValue v = 1.0; // from double
    EXPECT_TRUE(TinyJson::as<bool>(v));
  }
  {
    JsonValue v = 0.0;
    EXPECT_FALSE(TinyJson::as<bool>(v));
  }
  {
    JsonValue v = 2.0; // only 0/1 allowed for bool target
    EXPECT_THROW((void)TinyJson::as<bool>(v), std::out_of_range);
  }
}

// ---------- numeric → integral ----------
TEST(TinyJsonAs, NumericToIntegral) {
  {
    JsonValue v = 42.0;
    EXPECT_EQ(TinyJson::as<int>(v), 42);
  }
  {
    JsonValue v = -0.0;
    EXPECT_EQ(TinyJson::as<int>(v), 0);
  }
  {
    JsonValue v = 3.14; // truncation not allowed
    EXPECT_THROW((void)TinyJson::as<int>(v), std::bad_variant_access);
  }
  {
    JsonValue v = static_cast<double>(std::numeric_limits<int>::max()) + 1.0;
    EXPECT_THROW((void)TinyJson::as<int>(v), std::out_of_range);
  }
  {
    JsonValue v = static_cast<double>(std::numeric_limits<unsigned>::min()) - 1.0;
    EXPECT_THROW((void)TinyJson::as<unsigned>(v), std::out_of_range);
  }
}

// ---------- numeric → floating ----------
TEST(TinyJsonAs, NumericToFloating) {
  {
    JsonValue v = 3.141592653589793;
    EXPECT_DOUBLE_EQ(TinyJson::as<double>(v), 3.141592653589793);
  }
  {
    JsonValue v = 1e40; // outside float range
    EXPECT_THROW((void)TinyJson::as<float>(v), std::out_of_range);
  }
  {
    JsonValue v = -1e40;
    EXPECT_THROW((void)TinyJson::as<float>(v), std::out_of_range);
  }
}

// ---------- NaN / Inf handling ----------
TEST(TinyJsonAs, NaNAndInfinity) {
  {
    JsonValue v = std::numeric_limits<double>::quiet_NaN();
    float f = TinyJson::as<float>(v); // comparisons with NaN are false; your code allows it
    EXPECT_TRUE(std::isnan(f));
  }
  {
    JsonValue v = std::numeric_limits<double>::infinity();
    EXPECT_THROW((void)TinyJson::as<float>(v), std::out_of_range);  // inf > max
    EXPECT_THROW((void)TinyJson::as<double>(v), std::out_of_range); // also out of representable finite range
  }
  {
    JsonValue v = -std::numeric_limits<double>::infinity();
    EXPECT_THROW((void)TinyJson::as<float>(v), std::out_of_range);
    EXPECT_THROW((void)TinyJson::as<double>(v), std::out_of_range);
  }
}

// ---------- wrong-type (non-numeric) ----------
TEST(TinyJsonAs, WrongTypeThrows) {
  {
    JsonValue v = std::string("42");
    EXPECT_THROW((void)TinyJson::as<int>(v), std::bad_variant_access); // not a number in variant
  }
  {
    JsonValue v = true;
    EXPECT_THROW((void)TinyJson::as<std::string>(v), std::bad_variant_access);
  }
  {
    JsonValue v = nullptr;
    EXPECT_THROW((void)TinyJson::as<bool>(v), std::bad_variant_access);
  }
}

// ---------- default-value overload ----------
TEST(TinyJsonAs, DefaultValueOverload) {
  {
    JsonValue v = std::string("nope");
    EXPECT_EQ(TinyJson::as<int>(v, 7), 7); // wrong type → default
  }
  {
    JsonValue v = 3.5;
    EXPECT_EQ(TinyJson::as<int>(v, 9), 9); // truncation rejected → default
  }
  {
    JsonValue v = 1e40; // out of float range
    EXPECT_EQ(TinyJson::as<float>(v, 1.0f), 1.0f);
  }
  {
    JsonValue v = false;
    EXPECT_EQ(TinyJson::as<int>(v, -1), 0); // exact bool path → returns 0, not default
  }
}

// ---------- references / cv-qualifiers ----------
TEST(TinyJsonAs, CvRefNormalization) {
  const JsonValue v = std::string("x");
  // remove_cvref_t<T>
  const std::string s1 = TinyJson::as<const std::string>(v);
  EXPECT_EQ(s1, "x");
  std::string s2 = TinyJson::as<std::string const&>(v); // still returns by value
  EXPECT_EQ(s2, "x");
}