#include "pch.h"
#include "Utilities/StringUtilities.h"

// ===== StringUtilities Tests =====
// Verify that string_view/const-ref parameter changes produce identical results

class StringUtilitiesTest : public ::testing::Test {};

// ===== Split Tests =====

TEST_F(StringUtilitiesTest, Split_BasicComma) {
	auto result = StringUtilities::Split("a,b,c", ',');
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[0], "a");
	EXPECT_EQ(result[1], "b");
	EXPECT_EQ(result[2], "c");
}

TEST_F(StringUtilitiesTest, Split_NoDelimiter) {
	auto result = StringUtilities::Split("hello", ',');
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], "hello");
}

TEST_F(StringUtilitiesTest, Split_EmptyString) {
	auto result = StringUtilities::Split("", ',');
	ASSERT_EQ(result.size(), 1u);
	EXPECT_EQ(result[0], "");
}

TEST_F(StringUtilitiesTest, Split_TrailingDelimiter) {
	auto result = StringUtilities::Split("a,b,", ',');
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[2], "");
}

TEST_F(StringUtilitiesTest, Split_ConsecutiveDelimiters) {
	auto result = StringUtilities::Split("a,,b", ',');
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[1], "");
}

TEST_F(StringUtilitiesTest, Split_StringViewFromLiteral) {
	// Verify string_view parameter works with string literals
	auto result = StringUtilities::Split("key=value", '=');
	ASSERT_EQ(result.size(), 2u);
	EXPECT_EQ(result[0], "key");
	EXPECT_EQ(result[1], "value");
}

TEST_F(StringUtilitiesTest, Split_StringViewFromStdString) {
	// Verify string_view parameter works with std::string
	std::string s = "x:y:z";
	auto result = StringUtilities::Split(s, ':');
	ASSERT_EQ(result.size(), 3u);
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
	(void)StringUtilities::Trim(original);
	EXPECT_EQ(original, copy);
}

// ===== GetNthSegment Tests =====

TEST_F(StringUtilitiesTest, GetNthSegment_FirstSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegment("a|b|c", '|', 0), "a");
}

TEST_F(StringUtilitiesTest, GetNthSegment_MiddleSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegment("a|b|c", '|', 1), "b");
}

TEST_F(StringUtilitiesTest, GetNthSegment_LastSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegment("a|b|c", '|', 2), "c");
}

TEST_F(StringUtilitiesTest, GetNthSegment_OutOfRange) {
	EXPECT_EQ(StringUtilities::GetNthSegment("a|b", '|', 5), "");
}

TEST_F(StringUtilitiesTest, GetNthSegment_EmptyString) {
	EXPECT_EQ(StringUtilities::GetNthSegment("", '|', 0), "");
}

TEST_F(StringUtilitiesTest, GetNthSegment_SingleSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegment("hello", '|', 0), "hello");
}

TEST_F(StringUtilitiesTest, GetNthSegment_EmptySegments) {
	EXPECT_EQ(StringUtilities::GetNthSegment("a||c", '|', 1), "");
}

TEST_F(StringUtilitiesTest, GetNthSegment_MovieFrameFormat) {
	// Real movie frame format: "UDLRSsBA|........"
	EXPECT_EQ(StringUtilities::GetNthSegment("UDLRSsBA|........", '|', 0), "UDLRSsBA");
	EXPECT_EQ(StringUtilities::GetNthSegment("UDLRSsBA|........", '|', 1), "........");
}

// ===== GetNthSegmentView Tests =====

TEST_F(StringUtilitiesTest, GetNthSegmentView_FirstSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("a|b|c", '|', 0), "a");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_MiddleSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("a|b|c", '|', 1), "b");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_LastSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("a|b|c", '|', 2), "c");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_OutOfRange) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("a|b", '|', 5), "");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_EmptyString) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("", '|', 0), "");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_SingleSegment) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("hello", '|', 0), "hello");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_EmptySegments) {
	EXPECT_EQ(StringUtilities::GetNthSegmentView("a||c", '|', 1), "");
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_ViewPointsIntoSource) {
	// Verify zero-copy: the view should point into the source string's memory
	std::string source = "alpha|beta|gamma";
	std::string_view view = StringUtilities::GetNthSegmentView(source, '|', 1);
	EXPECT_EQ(view, "beta");
	EXPECT_GE(view.data(), source.data());
	EXPECT_LT(view.data(), source.data() + source.size());
}

TEST_F(StringUtilitiesTest, GetNthSegmentView_MatchesGetNthSegment) {
	// Both functions must return identical results
	std::string input = "UDLRSsBA|........|ABXY";
	for (size_t i = 0; i < 4; i++) {
		EXPECT_EQ(
			StringUtilities::GetNthSegmentView(input, '|', i),
			StringUtilities::GetNthSegment(input, '|', static_cast<int>(i))
		);
	}
}

// ===== CountSegments Tests =====

TEST_F(StringUtilitiesTest, CountSegments_ThreeSegments) {
	EXPECT_EQ(StringUtilities::CountSegments("a|b|c", '|'), 3u);
}

TEST_F(StringUtilitiesTest, CountSegments_OneSegment) {
	EXPECT_EQ(StringUtilities::CountSegments("hello", '|'), 1u);
}

TEST_F(StringUtilitiesTest, CountSegments_EmptyString) {
	EXPECT_EQ(StringUtilities::CountSegments("", '|'), 0u);
}

TEST_F(StringUtilitiesTest, CountSegments_ConsecutiveDelimiters) {
	EXPECT_EQ(StringUtilities::CountSegments("a||c", '|'), 3u);
}

TEST_F(StringUtilitiesTest, CountSegments_MovieFrameFormat) {
	// Real movie format: 2 controllers separated by pipe
	EXPECT_EQ(StringUtilities::CountSegments("UDLRSsBA|........", '|'), 2u);
}

TEST_F(StringUtilitiesTest, CountSegments_SingleDelimiter) {
	EXPECT_EQ(StringUtilities::CountSegments("|", '|'), 2u);
}

TEST_F(StringUtilitiesTest, CountSegments_MatchesSplitSize) {
	// CountSegments must match Split().size() for all inputs
	std::vector<std::string> inputs = {"", "a", "a|b", "a|b|c", "||", "a||b"};
	for (const auto& input : inputs) {
		EXPECT_EQ(
			StringUtilities::CountSegments(input, '|'),
			input.empty() ? 0u : StringUtilities::Split(input, '|').size()
		);
	}
}
