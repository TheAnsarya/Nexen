#include "pch.h"
#include "Utilities/StringUtilities.h"

// ===== StringUtilities Tests =====
// Verify that string_view/const-ref parameter changes produce identical results

class StringUtilitiesTest : public ::testing::Test {};

// ===== Split Tests =====

TEST_F(StringUtilitiesTest, Split_BasicComma) {
	auto result = StringUtilities::Split("a,b,c", ',');
	ASSERT_EQ(result.size(), 3);
	EXPECT_EQ(result[0], "a");
	EXPECT_EQ(result[1], "b");
	EXPECT_EQ(result[2], "c");
}

TEST_F(StringUtilitiesTest, Split_NoDelimiter) {
	auto result = StringUtilities::Split("hello", ',');
	ASSERT_EQ(result.size(), 1);
	EXPECT_EQ(result[0], "hello");
}

TEST_F(StringUtilitiesTest, Split_EmptyString) {
	auto result = StringUtilities::Split("", ',');
	ASSERT_EQ(result.size(), 1);
	EXPECT_EQ(result[0], "");
}

TEST_F(StringUtilitiesTest, Split_TrailingDelimiter) {
	auto result = StringUtilities::Split("a,b,", ',');
	ASSERT_EQ(result.size(), 3);
	EXPECT_EQ(result[2], "");
}

TEST_F(StringUtilitiesTest, Split_ConsecutiveDelimiters) {
	auto result = StringUtilities::Split("a,,b", ',');
	ASSERT_EQ(result.size(), 3);
	EXPECT_EQ(result[1], "");
}

TEST_F(StringUtilitiesTest, Split_StringViewFromLiteral) {
	// Verify string_view parameter works with string literals
	auto result = StringUtilities::Split("key=value", '=');
	ASSERT_EQ(result.size(), 2);
	EXPECT_EQ(result[0], "key");
	EXPECT_EQ(result[1], "value");
}

TEST_F(StringUtilitiesTest, Split_StringViewFromStdString) {
	// Verify string_view parameter works with std::string
	std::string s = "x:y:z";
	auto result = StringUtilities::Split(s, ':');
	ASSERT_EQ(result.size(), 3);
	EXPECT_EQ(result[0], "x");
	EXPECT_EQ(result[1], "y");
	EXPECT_EQ(result[2], "z");
}

// ===== Trim Tests =====

TEST_F(StringUtilitiesTest, Trim_Whitespace) {
	EXPECT_EQ(StringUtilities::Trim("  hello  "), "hello");
}

TEST_F(StringUtilitiesTest, Trim_NoWhitespace) {
	EXPECT_EQ(StringUtilities::Trim("hello"), "hello");
}

TEST_F(StringUtilitiesTest, Trim_AllWhitespace) {
	EXPECT_EQ(StringUtilities::Trim("   "), "");
}

TEST_F(StringUtilitiesTest, Trim_Empty) {
	EXPECT_EQ(StringUtilities::Trim(""), "");
}

TEST_F(StringUtilitiesTest, TrimLeft_Whitespace) {
	EXPECT_EQ(StringUtilities::TrimLeft("  hello  "), "hello  ");
}

TEST_F(StringUtilitiesTest, TrimRight_Whitespace) {
	EXPECT_EQ(StringUtilities::TrimRight("  hello  "), "  hello");
}

TEST_F(StringUtilitiesTest, Trim_ConstRef_NoMutation) {
	// Verify the const& parameter doesn't modify the original
	std::string original = "  test  ";
	std::string copy = original;
	StringUtilities::Trim(original);
	EXPECT_EQ(original, copy);
}
